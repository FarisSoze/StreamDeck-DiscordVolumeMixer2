#include <QCoreApplication>
#include <qtdiscordipc/qdiscord.h>
#include <cstdio>

// An isolated client: no Stream Deck settings or Windows registry access.
// The test harness supplies synthetic cached tokens in a temporary directory
// and runs a fake Discord IPC endpoint. No real user credentials are read.
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    // Windows GUI-subsystem builds otherwise send Qt messages to the debugger,
    // not the pipe observed by the test harness.
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
        std::fprintf(stderr, "%s\n", message.toUtf8().constData());
        std::fflush(stderr);
    });
    QDiscord discord;
    if(!discord.connect("recovery-test-client", "recovery-test-secret"))
        return 1;
    return discord.userID() == "recovery-test-user" ? 0 : 2;
}
