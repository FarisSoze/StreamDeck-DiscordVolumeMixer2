#include "action_openmixer.h"

#include "dvmplugin.h"

Action_OpenMixer::Action_OpenMixer() {
	connect(this, &QStreamDeckAction::keyDown, this, &Action_OpenMixer::onPressed);
}

void Action_OpenMixer::onPressed() {
	using DT = QStreamDeckDevice::DeviceType;
	static const QMap<int, QString> profileNameByDeviceType{
		{+DT::streamDeck,       "Voice Mixer Plus FIX"},
		{+DT::streamDeckMini,   "Voice Mixer Plus FIX Mini"},
		{+DT::streamDeckXL,     "Voice Mixer Plus FIX XL"},
		{+DT::streamDeckMobile, "Voice Mixer Plus FIX"},
		{+DT::streamDeckPlus,   "Voice Mixer Plus FIX+"},
	};

	device()->switchToProfile(profileNameByDeviceType.value(+device()->deviceType(), "Voice Mixer Plus FIX"));
	plugin()->updateChannelMembersData();
}
