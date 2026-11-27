#pragma once

#include <qtstreamdeck2/qstreamdeckplugin.h>
#include <qtdiscordipc/qdiscord.h>
#include <QSettings>

#include "declares.h"
#include "voicechannelmember.h"
#include "dvmdevice.h"

class DVMPlugin : public QStreamDeckPluginT<DVMDevice> {
Q_OBJECT

public:
	DVMPlugin();
	~DVMPlugin();

public slots:
	/// Attempts to connect to Discord
	void connectToDiscord();
	void reconnectToDiscord();

	/// Reloads all channel member data
	void updateChannelMembersData();

public:
	/// Processes voice state update for the user itself
	void updateSelfVoiceState(const QDiscordMessage &msg);

	void adjustVoiceChannelMemberVolume(VoiceChannelMember &vcm, float stepSize, int numSteps);

public:
	QString connectionState() const { return connectionState_; }
	QString connectionReason() const { return connectionReason_; }

	QDiscord discord;

public:
	QString currentVoiceChannelID;
	QMap<QString, VoiceChannelMember> voiceChannelMembers;
	QSet<QString> speakingVoiceChannelMembers;

	int voiceChannelMemberIxOffset = 0;

public:
	bool isDeafened = false;
	bool isMicrophoneMuted = false;

signals:
	void connectionStatusChanged();
	/// Updates text & states of all user related buttons
	void buttonsUpdateRequested();

private:
	void updateCurrentVoiceChannel(const QString &newVoiceChannel);
	void setConnectionStatus(const QString &state, const QString &reason = {});

private slots:
	void onInitialized();
	void onDiscordMessageReceived(const QDiscordMessage &msg);
	void onStreamDeckEventReceived(const QStreamDeckEvent &e);

private:
	bool discordConnectionInProgress_ = false;
	bool discordReconnectScheduled_ = false;
	QString connectionState_ = "connecting";
	QString connectionReason_;
	QTimer discordConnectTimeoutTimer_;
	QTimer discordReconnectTimer_;

};
