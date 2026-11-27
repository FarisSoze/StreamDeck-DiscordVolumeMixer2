#include <QCoreApplication>
#include <qtdiscordipc/qdiscord.h>

// An isolated client: no Stream Deck settings or Windows registry access.
// The test harness supplies synthetic cached tokens in a temporary directory
// and runs a fake Discord IPC endpoint. No real user credentials are read.
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QDiscord discord;
    if(!discord.connect("recovery-test-client", "recovery-test-secret"))
        return 1;
    return discord.userID() == "recovery-test-user" ? 0 : 2;
}
