# Set up Voice Mixer Plus — FIX

Windows 10/11 x64, desktop Discord, and Stream Deck 6.9 or later are required.
Stream Deck+ dials are supported. This is an independent community fork and
installs alongside Danol's original plugin; existing original actions are not
automatically replaced.

1. Install the `.streamDeckPlugin` package, then add **Open Voice Mixer** from
   **Voice Mixer Plus — FIX** to a key.
2. Open https://discord.com/developers/applications and create an application
   using the same account as your desktop Discord client. Use a name such as
   **Voice Mixer**; avoid naming the application “Discord”. This is a personal
   application integration, not a Discord bot.
3. Under Installation choose **Discord Provided Link**, and save. Under OAuth2,
   add the redirect `http://localhost:1337/callback`, then save.
4. Copy your own Client ID and Client secret into the action's settings. Never
   share the secret or your local `discordOauth.json` file.
5. Open desktop Discord. Click **إعادة الاتصال / Reconnect** and approve the
   integration in Discord if asked. Status should become **متصل** (Connected).
6. Join a voice channel and press the mixer key on your deck. Adjust each other
   participant's volume, mute individual users, or use the supported dials.

Connection labels: **متصل** = Connected; **جاري الاتصال** = Connecting;
**تعذّر الاتصال** = Connection failed. An explanation appears when a request
fails. Reconnect retries the local integration without restarting Stream Deck.

If Discord and Stream Deck run at different privilege levels, close and reopen
them normally. If no other members are in your voice channel, the mixer has no
other voices to show. Never post credentials in support issues.

## الإعداد بالعربي

ثبّت الإضافة وأضف زر **Open Voice Mixer**. أنشئ تطبيقًا خاصًا بك في بوابة
مطوّري Discord بنفس حساب الديسكورد، وأضف رابط التحويل الموضح أعلاه. أدخل
Client ID وClient secret في إعدادات الزر، ثم اضغط **إعادة الاتصال / Reconnect**
ووافق داخل Discord إن ظهر طلب الإذن. ادخل رومًا صوتيًا واضغط زر الميكسر.

هذه نسخة مستقلة؛ أزرار الإضافة الأصلية لا تتحول إليها تلقائيًا. بيانات
الاتصال تبقى محليًا، ولا تأتي أي مفاتيح أو حسابات ضمن حزمة التثبيت.

Based on Danol's GPL-3.0 Discord Volume Mixer. Maintained by FarisSoze.
