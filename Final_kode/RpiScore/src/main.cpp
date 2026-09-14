#include <iostream>
#include <csignal>
#include "RpiScore.hpp"

static RpiScore* g_instance = nullptr;

void signalHandler(int)
{
    if (g_instance)
        g_instance->saveHighscore();
    std::exit(0);
}

int main(int argc, char* argv[])
{
    std::string serverIp = "127.0.0.1";
    if (argc > 1)
    {
        serverIp = argv[1];
    }
    else
    {
        std::cout << "[Main] Ingen IP angivet, bruger localhost (kun til test)\n";
    }

    std::cout << "[Main] Connecting to server at " << serverIp << "\n";

    RpiScore client(serverIp);
    g_instance = &client;
    std::signal(SIGINT, signalHandler); // når vi stopper programmet - gør det her (kald signalHandler)

    client.run(); // opstart 

    return 0;
}
