# Privacy and local storage

Discord Voice Mixer FIX runs locally and connects to the desktop Discord client
using Discord IPC. Discord OAuth requests go to Discord; avatars are fetched
from Discord's CDN. No FarisSoze server, analytics, or telemetry is used.

Your own Discord application Client ID and Client secret are required. They
are stored in your Windows user settings. OAuth tokens are stored locally in
`discordOauth.json` in the plugin folder. Keep these local settings and files
private. The installer does not contain anyone's credentials.

Diagnostics are off unless local logging files are explicitly created. Logs may
contain account and voice-channel information. Review and redact diagnostic
files before sharing them. Never upload your local plugin directory as a bug
report; report the version, symptom and error message instead.

Source: https://github.com/FarisSoze/StreamDeck-DiscordVolumeMixer2
Support: https://github.com/FarisSoze/StreamDeck-DiscordVolumeMixer2/issues
