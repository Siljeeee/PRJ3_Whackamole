#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <vector>
#include <random>
#include <atomic>
#include "LED.h"
#include "ForceSensor.h"
#include "GameLogic.h"

struct GameMessage
{
    std::string type;
    int score = 0;
    int round = 0;
    int activeLED = 0;
    int timeLeft = 0;
};

class GameLogic
{
public:
    GameLogic(
        LED& led,
        ForceSensor& sensor,
        int numLEDs,
        int numberOfRounds,
        int roundDurationMs,
        int pauseBetweenRoundsMs,
        const std::vector<int>& ledTimePerRoundMs
    );

    void GameLogicThread();

    void requestStart();
    void requestShutdown();
    bool isRunning() const;

    bool waitForMessage(GameMessage& msg);

private:
    void waitForStart();
    void startGame();
    void runRound(int roundNumber, int ledTimeMs);
    void countdownToNextRound(int nextRound);
    void gameOver();

    int  chooseRandomLED();
    void sendMessage(const std::string& type, int timeLeft = 0);

    LED& led_;
    ForceSensor& sensor_;

    int numLEDs_;
    int numberOfRounds_;
    int roundDurationMs_;
    int pauseBetweenRoundsMs_;
    std::vector<int> ledTimePerRoundMs_;

    std::atomic<bool> running_{true};

    mutable std::mutex mtx_;
    std::condition_variable cvStart_;
    std::condition_variable cvSendData_;

    bool startRequested_ = false;
    bool gameActive_ = false;

    int score_ = 0;
    int round_ = 0;
    int activeLED_ = 0;

    std::queue<GameMessage> messageQueue_;
    std::mt19937 rng_{std::random_device{}()};
};

#endif
