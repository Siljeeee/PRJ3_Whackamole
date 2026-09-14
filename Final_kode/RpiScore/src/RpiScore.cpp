#include "RpiScore.hpp"
#include <iostream>
#include <fstream>
#include <json_dto/pub.hpp>

static const std::string HIGHSCORE_PATH = "../highscore.txt";

RpiScore::RpiScore(const std::string & serverIp)
    : receiveData_(serverIp), highscore_(0) {}

void RpiScore::loadHighscore()
{
    std::ifstream file(HIGHSCORE_PATH);
    if (file.is_open())
        file >> highscore_;
}

void RpiScore::run()
{
    loadHighscore();
    displayServer_.start();

    if (!receiveData_.connect())
    {
        std::cerr << "[RpiScore] Could not connect to server.\n";
        return;
    }

    // Signal til RpiGamelogic at display er klar
    receiveData_.parseDisplayData();
    std::cout << "[RpiScore] Connected and ready.\n";

    GameState_t state;
    while (receiveData_.parseData(state))
    {
        updateHighscore(state.score);

        // Sæt den lokalt kendte highscore før vi sender videre
        state.highScore = highscore_;

        displayServer_.sendJson(json_dto::to_json(state));
        receiveData_.sendDisplayUpdated();
    }

    saveHighscore();
}

void RpiScore::saveHighscore()
{
    std::ofstream file(HIGHSCORE_PATH);
    if (file.is_open())
    {
        file << highscore_;
        std::cout << "[RpiScore] Highscore saved: " << highscore_ << "\n";
    }
    else
    {
        std::cerr << "[RpiScore] Could not save highscore to " << HIGHSCORE_PATH << "\n";
    }
}

void RpiScore::updateHighscore(int score)
{
    if (score > highscore_)
    {
        highscore_ = score;
        saveHighscore();
    }
}