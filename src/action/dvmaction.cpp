#include "dvmaction.h"

#include <qtstreamdeck2/qstreamdeckpropertyinspectorbuilder.h>

#include "dvmplugin.h"

DVMAction::DVMAction() {
	connect(this, &QStreamDeckAction::initialized, this, &DVMAction::onInitialized);
	connect(this, &QStreamDeckAction::settingsChanged, this, &DVMAction::update);
}

void DVMAction::buildPropertyInspector(QStreamDeckPropertyInspectorBuilder &b) {
	const QString state = plugin()->connectionState();
	const QString label = state == "connected" ? QStringLiteral("متصل")
	    : state == "connecting" ? QStringLiteral("جاري الاتصال…") : QStringLiteral("تعذّر الاتصال");
	b.addHTML(QStringLiteral(
	    "<section class=\"dvm-connection\" dir=\"rtl\" aria-label=\"اتصال Discord\">"
	    "<strong id=\"discordConnectionLabel\" role=\"status\" aria-live=\"polite\">%1</strong>"
	    "<p id=\"discordConnectionReason\"%2>%3</p>"
	    "<button id=\"discordReconnect\" type=\"button\" onclick=\"reconnectDiscord()\"%4>إعادة الاتصال / <bdi lang=\"en\">Reconnect</bdi></button>"
	    "</section>")
	    .arg(label, plugin()->connectionReason().isEmpty() ? " hidden" : "",
	         plugin()->connectionReason().toHtmlEscaped(), state == "connecting" ? " disabled" : ""));
	b.addSection("Discord settings");
	b.addLineEdit("client_id", "Client ID").linkWithGlobalSetting();
	b.addLineEdit("client_secret", "Client secret").linkWithGlobalSetting();
	b.addMessage("For configuration instructions, see the <a href=\"javascript: openUrl('https://github.com/FarisSoze/StreamDeck-DiscordVolumeMixer2');\">GitHub page</a>.");
}

void DVMAction::onInitialized() {
	connect(plugin(), &DVMPlugin::connectionStatusChanged, this, [this] {
		sendMessage("sendToPropertyInspector", {
			{"cmd", "updateDiscordConnection"},
			{"state", plugin()->connectionState()},
			{"reason", plugin()->connectionReason()},
		});
	});
	connect(this, &QStreamDeckAction::eventReceived, this, [this](const QStreamDeckEvent &event) {
		if(event.eventType == QStreamDeckEvent::EventType::sendToPlugin && event.payload["cmd"] == "reconnectDiscord")
			plugin()->reconnectToDiscord();
	});
	connect(plugin(), &DVMPlugin::buttonsUpdateRequested, this, &DVMAction::update);
	connect(plugin(), &DVMPlugin::globalSettingsChanged, this, &DVMAction::update);

	QTimer::singleShot(0, this, &DVMAction::update);
}
