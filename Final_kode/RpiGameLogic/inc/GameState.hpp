#pragma once
#include <string>
#include <json_dto/pub.hpp>

struct GameState_t
{
    int score = 0;
    int round = 0;
    int time = 0;
    int highScore = 0;
    int countdown = 0;
    std::string gameState = "idle";

    template <typename JSON_IO>
    void json_io(JSON_IO & io)
    {
        io
        & json_dto::mandatory("score", score)
        & json_dto::mandatory("round", round)
        & json_dto::mandatory("time", time)
        & json_dto::mandatory("gameState", gameState)
        & json_dto::optional("highScore", highScore, 0)
        & json_dto::optional("countdown", countdown, 0);
    }
};
