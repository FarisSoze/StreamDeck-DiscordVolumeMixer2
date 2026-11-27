#include "action_openmixer.h"

#include "dvmplugin.h"

Action_OpenMixer::Action_OpenMixer() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_OpenMixer::onPressed);
}

void Action_OpenMixer::onPressed() {
	using DT = QStreamDeckDevice::DeviceType;
	static const QMap<int, QString> profileNameByDeviceType{
		{+DT::streamDeck,       "Discord Voice Mixer FIX"},
		{+DT::streamDeckMini,   "Discord Voice Mixer FIX Mini"},
		{+DT::streamDeckXL,     "Discord Voice Mixer FIX XL"},
		{+DT::streamDeckMobile, "Discord Voice Mixer FIX"},
		{+DT::streamDeckPlus,   "Discord Voice Mixer FIX+"},
	};

	device()->switchToProfile(profileNameByDeviceType.value(+device()->deviceType(), "Discord Voice Mixer FIX"));
	plugin()->updateChannelMembersData();
}
