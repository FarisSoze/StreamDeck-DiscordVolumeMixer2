#include "dvmplugin.h"

#include "dvmdevice.h"
#include <QScopedValueRollback>

#include "action/action_openmixer.h"
#include "action/action_vcminfo.h"
#include "action/action_vcmvolume.h"
#include "action/action_vcmpaging.h"
#include "action/action_back.h"
#include "action/action_deafen.h"
#include "action/action_microphone.h"

DVMPlugin::DVMPlugin() {
	registerActionType<Action_OpenMixer>("cz.danol.discordmixer.openmixer");
	registerActionType<Action_VCMInfo>("cz.danol.discordmixer.user");
	registerActionType<Action_VCMVolume>("cz.danol.discordmixer.volumeup");
	registerActionType<Action_VCMVolume>("cz.danol.discordmixer.volumedown");
	registerActionType<Action_VCMPaging>("cz.danol.discordmixer.nextpage");
	registerActionType<Action_VCMPaging>("cz.danol.discordmixer.previouspage");
	registerActionType<Action_Back>("cz.danol.discordmixer.back");
	registerActionType<Action_Microphone>("cz.danol.discordmixer.microphone");
	registerActionType<Action_Deafen>("cz.danol.discordmixer.deafen");

	connect(this, &QStreamDeckPlugin::initialized, this, &DVMPlugin::onInitialized);
	connect(this, &QStreamDeckPlugin::eventReceived, this, &DVMPlugin::onStreamDeckEventReceived);

	connect(&discord, &QDiscord::messageReceived, this, &DVMPlugin::onDiscordMessageReceived);
	connect(&discord, &QDiscord::avatarReady, this, &DVMPlugin::buttonsUpdateRequested);
	connect(&discord, &QDiscord::disconnected, this, [this] {
		currentVoiceChannelID.clear();
		voiceChannelMembers.clear();
		speakingVoiceChannelMembers.clear();
		voiceChannelMemberIxOffset = 0;
		if(!discordConnectionInProgress_ && !discordReconnectScheduled_)
			setConnectionStatus("error", QStringLiteral("انقطع الاتصال بالديسكورد. ستتم إعادة المحاولة تلقائيًا."));

		discordReconnectTimer_.start();

		emit buttonsUpdateRequested();
	});

	discordConnectTimeoutTimer_.setSingleShot(true);
	discordConnectTimeoutTimer_.setInterval(2000);

	discordReconnectTimer_.setInterval(2500);
	discordReconnectTimer_.callOnTimeout(this, &DVMPlugin::connectToDiscord);
	discordReconnectTimer_.start();
}

DVMPlugin::~DVMPlugin() {

}

void DVMPlugin::connectToDiscord() {
	if(discord.isConnected() || discordConnectionInProgress_ || discordReconnectScheduled_)
		return;
	// OAuth uses a nested event loop. Timer ticks and key presses must not
	// start another handshake on the same IPC socket while it is in progress.
	QScopedValueRollback<bool> connecting(discordConnectionInProgress_, true);
	setConnectionStatus("connecting");

	if(discord.connect(globalSetting("client_id").toString(), globalSetting("client_secret").toString())) {
		// Subscribe to voice channel select event
		discord.sendCommand(+QDiscord::CommandType::subscribe, {}, QJsonObject{
			{"evt", "VOICE_CHANNEL_SELECT"},
		});

		{
			auto r = discord.sendCommand(+QDiscord::CommandType::getVoiceSettings);
			connect(r, &QDiscordReply::success, this, [this](const QDiscordMessage &msg) {
				isMicrophoneMuted = msg.data["mute"].toBool();
				isDeafened = msg.data["deaf"].toBool();
				emit buttonsUpdateRequested();
			});
		}

		updateChannelMembersData();
		discordReconnectTimer_.stop();
		setConnectionStatus("connected");
	}
	else {
		const QString error = discord.connectionError();
		QString reason;
		if(error == "ERR 0")
			reason = QStringLiteral("أدخل Client ID وClient secret في إعدادات الإضافة، ثم أعد الاتصال.");
		else if(error == "ERR 1")
			reason = QStringLiteral("تعذّر الوصول إلى Discord. افتح تطبيق Discord وتأكد أن البرنامجين يعملان بنفس مستوى الصلاحيات.");
		else if(error == "ERR 2")
			reason = QStringLiteral("رفض Discord بيانات الاتصال. تحقق من إعدادات التطبيق في بوابة المطوّرين.");
		else if(error == "ERR 3" || error == "ERR 5")
			reason = QStringLiteral("تعذّر تجديد صلاحية الاتصال. تحقق من الإنترنت ثم أعد المحاولة.");
		else if(error == "ERR 4")
			reason = QStringLiteral("لم تكتمل الموافقة على الاتصال. تحقق من طلب الإذن داخل Discord ومن الحساب المستخدم.");
		else if(error == "ERR 6" || error == "ERR 7")
			reason = QStringLiteral("لم يقبل Discord صلاحية الاتصال. تحقق من بيانات التطبيق ثم أعد المحاولة.");
		else if(error == "ERR 8")
			reason = QStringLiteral("لم يستجب Discord لبدء الاتصال. تأكد أن التطبيق جاهز ثم أعد المحاولة.");
		else
			reason = QStringLiteral("تعذّر الاتصال بالديسكورد. تأكد أن التطبيق مفتوح ثم أعد المحاولة.");
		setConnectionStatus("error", reason);
		emit buttonsUpdateRequested();
		discordReconnectTimer_.start();
	}
}

void DVMPlugin::setConnectionStatus(const QString &state, const QString &reason) {
	if(connectionState_ == state && connectionReason_ == reason)
		return;
	connectionState_ = state;
	connectionReason_ = reason;
	emit connectionStatusChanged();
}

void DVMPlugin::reconnectToDiscord() {
	if(discordConnectionInProgress_ || discordReconnectScheduled_)
		return;
	discordReconnectScheduled_ = true;
	setConnectionStatus("connecting");
	discord.disconnect();
	discordReconnectTimer_.stop();
	// Let the inspector paint its pending state before the blocking IPC handshake.
	QTimer::singleShot(0, this, [this] {
		discordReconnectScheduled_ = false;
		connectToDiscord();
	});
}

void DVMPlugin::updateChannelMembersData() {
	QDiscordReply *r = discord.sendCommand(+QDiscord::CommandType::getSelectedVoiceChannel);
	connect(r, &QDiscordReply::success, this, [this](const QDiscordMessage &msg) {
		updateCurrentVoiceChannel(msg.data["id"].toString());

		// Update voice channel member list
		{
			const auto arr = msg.data["voice_states"].toArray();

			voiceChannelMembers.clear();

			for(const auto &v: arr) {
				const VoiceChannelMember vs = VoiceChannelMember::fromJson(v.toObject());
				if(vs.userID != discord.userID())
					voiceChannelMembers.insert(vs.userID, vs);
			}
		}

		if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
			voiceChannelMemberIxOffset = 0;

		emit buttonsUpdateRequested();
	});
}

void DVMPlugin::updateSelfVoiceState(const QDiscordMessage &msg) {
	const QJsonObject json = msg.data;

	if(auto v = json["voice_state"]["self_mute"]; !v.isNull())
		isMicrophoneMuted = v.toBool();

	if(auto v = json["voice_state"]["self_deaf"]; !v.isNull())
		isDeafened = v.toBool();
}

void DVMPlugin::adjustVoiceChannelMemberVolume(VoiceChannelMember &vcm, float stepSize, int numSteps) {
	const float step = globalSetting("voiceChannelVolumeButtonStep").toInt();
	float newVolume = vcm.volume + stepSize * numSteps;
	newVolume = qBound(QDiscord::minVoiceVolume, newVolume, QDiscord::maxVoiceVolume);
	newVolume = qRound(newVolume / stepSize) * stepSize;

	if(newVolume != vcm.volume || vcm.isMuted) {
		vcm.volume = newVolume;
		vcm.isMuted = false;

		discord.sendCommand(+QDiscord::CommandType::setUserVoiceSettings, QJsonObject{
			{"user_id", vcm.userID},
			{"volume",  QDiscord::uiToIPCVolume(newVolume)},
			{"mute",    false},
		});
		emit buttonsUpdateRequested();
	}
}

void DVMPlugin::updateCurrentVoiceChannel(const QString &newVoiceChannel) {
	// If the channel changed, update event subscribtions
	if(newVoiceChannel == currentVoiceChannelID)
		return;

	static const QStringList events{
		"VOICE_STATE_UPDATE", "VOICE_STATE_CREATE", "VOICE_STATE_DELETE", "SPEAKING_START", "SPEAKING_STOP"
	};
	const auto evf = [&](const QString &cmd) {
		const QJsonObject args{{"channel_id", currentVoiceChannelID}};
		for(const QString &e: events)
			discord.sendCommand(cmd, args, {{"evt", e}});
	};

	if(!currentVoiceChannelID.isEmpty())
		evf(+QDiscord::CommandType::unsubscribe);

	currentVoiceChannelID = newVoiceChannel;

	if(!currentVoiceChannelID.isEmpty())
		evf(+QDiscord::CommandType::subscribe);
}

void DVMPlugin::onDiscordMessageReceived(const QDiscordMessage &msg) {
	using ET = QDiscordMessage::EventType;

	switch(msg.event) {

		// Voice channel changed
		case ET::voiceChannelSelect:
			updateCurrentVoiceChannel(msg.data["channel_id"].toString());
			updateChannelMembersData();
			break;

		case ET::voiceStateCreate: {
			const auto m = VoiceChannelMember::fromJson(msg.data);
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateUpdate: {
			const auto m = VoiceChannelMember::fromJson(msg.data);
			if(m.userID == discord.userID())
				updateSelfVoiceState(msg);

			else if(voiceChannelMembers.contains(m.userID))
				voiceChannelMembers.insert(m.userID, m);

			break;
		}

		case ET::voiceStateDelete: {
			const auto voiceData = VoiceChannelMember::fromJson(msg.data);
			voiceChannelMembers.remove(voiceData.userID);
			if(voiceChannelMemberIxOffset >= voiceChannelMembers.size())
				voiceChannelMemberIxOffset = 0;

			/*
			 * Bug workaround - when voice state delete reports the current user, it possibly means that the user has been moved by admin to another voice channel, which does not trigger the VOICE_CHANNEL_SELECT event.
			 * So in this case, we requery everything.
			 * */
			if(voiceData.userID == discord.userID())
				updateChannelMembersData();

			break;
		}

		case ET::speakingStart:
			speakingVoiceChannelMembers.insert(msg.data["user_id"].toString());
			break;

		case ET::speakingStop:
			speakingVoiceChannelMembers.remove(msg.data["user_id"].toString());
			break;

		case ET::voiceSettingsUpdate:
			isMicrophoneMuted = msg.data["mute"].toBool();
			isDeafened = msg.data["deaf"].toBool();
			emit buttonsUpdateRequested();
			break;

		default:
			return;

	}

	emit buttonsUpdateRequested();
}

void DVMPlugin::onInitialized() {
	setGlobalSettingDefault("voiceChannelVolumeButtonStep", 5);
	setGlobalSettingDefault("voiceChannelVolumeEncoderStep", 5);

	connectToDiscord();
}

void DVMPlugin::onStreamDeckEventReceived(const QStreamDeckEvent &e) {
	using ET = QStreamDeckEvent::EventType;

	// Try connecting to discord whenever any button is pressed
	if(!discord.isConnected() && !discordConnectTimeoutTimer_.isActive() && (e.eventType == ET::touchTap || e.eventType == ET::keyDown || e.eventType == ET::dialDown || e.eventType == ET::dialUp || e.eventType == ET::dialRotate)) {
		discordConnectTimeoutTimer_.start();
		connectToDiscord();
	}
}
