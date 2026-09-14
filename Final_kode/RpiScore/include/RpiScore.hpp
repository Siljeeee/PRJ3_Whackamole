// EFTER
#pragma once
#include "ReceiveData.hpp"
#include "DisplayServer.hpp"
#include "GameState.hpp"

class RpiScore
{
public:
    RpiScore(const std::string & serverIp);

    void run();
    void saveHighscore();

private:
    ReceiveData   receiveData_;
    DisplayServer displayServer_;
    int           highscore_;

    void updateHighscore(int score);
    void loadHighscore();
};