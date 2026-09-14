#ifndef TERMINAL_H
#define TERMINAL_H
#include "GameLogic.h"


class Terminal
{
public:
    explicit Terminal(GameLogic& gameLogic);
    void TerminalThread();

private:
    GameLogic& gameLogic_;
};

#endif
