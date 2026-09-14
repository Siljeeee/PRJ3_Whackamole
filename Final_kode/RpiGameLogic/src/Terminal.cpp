#include "Terminal.h"
#include "GameLogic.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

Terminal::Terminal(GameLogic& gameLogic)
    : gameLogic_(gameLogic)
{
}

void Terminal::TerminalThread()
{
    std::cout << "Terminal ready. Write 'start' to start game, 'quit' to shutdown.\n";

    while (gameLogic_.isRunning())
    {
        std::string input;
        std::getline(std::cin, input);
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);

        if (input == "start")
            gameLogic_.requestStart();
        else if (input == "quit")
        {
            gameLogic_.requestShutdown();
            break;
        }
        else if (!input.empty())
            std::cout << "Unknown command. Use 'start' or 'quit'.\n";
    }

    std::cout << "Terminal stopped\n";
}
