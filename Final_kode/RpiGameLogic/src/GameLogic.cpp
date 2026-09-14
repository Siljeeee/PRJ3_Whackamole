#include "GameLogic.h"
#include "LED.h"
#include "ForceSensor.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

GameLogic::GameLogic(
    LED& led,
    ForceSensor& sensor,
    int numLEDs,
    int numberOfRounds,
    int roundDurationMs,
    int pauseBetweenRoundsMs,
    const std::vector<int>& ledTimePerRoundMs
)
    : led_(led), sensor_(sensor),
      numLEDs_(numLEDs),
      numberOfRounds_(numberOfRounds),
      roundDurationMs_(roundDurationMs),
      pauseBetweenRoundsMs_(pauseBetweenRoundsMs),
      ledTimePerRoundMs_(ledTimePerRoundMs)
{
    led_.turnOffAll();
}

// ─── Main loop ───────────────────────────────────────────────────────────────

void GameLogic::GameLogicThread()
{
    while (running_.load())
    {
        waitForStart();
        if (!running_.load()) break;

        startGame();

        for (int r = 1; r <= numberOfRounds_ && running_.load() && gameActive_; r++)
        {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                round_ = r;
            }

            sendMessage("start_round");
            runRound(r, ledTimePerRoundMs_[r - 1]);
            led_.turnOffAll();

            if (r < numberOfRounds_ && running_.load() && gameActive_)
                countdownToNextRound(r + 1);
        }

        if (running_.load())
            gameOver();
    }

    led_.turnOffAll();
}

// ─── Game phases ─────────────────────────────────────────────────────────────

void GameLogic::waitForStart()
{
    std::unique_lock<std::mutex> lock(mtx_);
    std::cout << "Write 'start' to start a game or 'quit' to shutdown.\n";
    cvStart_.wait(lock, [&] { return startRequested_ || !running_.load(); });
    startRequested_ = false;
}

void GameLogic::startGame()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        score_ = 0;
        round_ = 0;
        activeLED_ = 0;
        gameActive_ = true;
    }

    led_.blinkAll(3);
    sendMessage("start_game");
    countdownToNextRound(1);
}

void GameLogic::runRound(int roundNumber, int ledTimeMs)
{
    using namespace std::chrono;

    auto roundEnd = steady_clock::now() + milliseconds(roundDurationMs_);
    int  lastPrintedSec = -1;

    while (running_.load() && gameActive_)
    {
        auto now = steady_clock::now();
        int  remainingMs = static_cast<int>(
            duration_cast<milliseconds>(roundEnd - now).count()
        );

        if (remainingMs <= 0) break;

        int timeLeftSec = (remainingMs + 999) / 1000;

        if (timeLeftSec != lastPrintedSec)
        {
            lastPrintedSec = timeLeftSec;
            sendMessage("timer_update", timeLeftSec);
        }

        int timeout = std::min(ledTimeMs, remainingMs);

        {
            std::lock_guard<std::mutex> lock(mtx_);
            activeLED_ = chooseRandomLED();
        }

        led_.activate(activeLED_);
        sendMessage("led_active");

        int hit = sensor_.scanWithinTime(timeout, running_);

        led_.turnOffAll();

        if (hit >= 0)
        {
            if (hit == activeLED_)
            {
                int s;
                {
                    std::lock_guard<std::mutex> lock(mtx_);
                    score_++;
                    s = score_;
                }
                sendMessage("update_point");
            }
            else
            {
                sendMessage("miss");
            }

            // Wait until the player lifts the hammer before picking the next LED.
            while (running_.load() && sensor_.anyActive())
                std::this_thread::sleep_for(milliseconds(20));

            std::this_thread::sleep_for(milliseconds(100));
        }
        else
        {
            sendMessage("miss");
            std::this_thread::sleep_for(milliseconds(100));
        }
    }
}

void GameLogic::countdownToNextRound(int nextRound)
{
    led_.turnOffAll();

    int seconds = pauseBetweenRoundsMs_ / 1000;
    for (int i = seconds; i > 0 && running_.load(); i--)
    {
        sendMessage("countdown", i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void GameLogic::gameOver()
{
    int finalScore;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        gameActive_ = false;
    }

    sendMessage("game_over");
}

// ─── Public interface ─────────────────────────────────────────────────────────

void GameLogic::requestStart()
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!gameActive_) startRequested_ = true;
    }
    cvStart_.notify_one();
}

void GameLogic::requestShutdown()
{
    running_.store(false);
    {
        std::lock_guard<std::mutex> lock(mtx_);
        gameActive_ = false;
    }
    cvStart_.notify_all();
    cvSendData_.notify_all();
}

bool GameLogic::isRunning() const
{
    return running_.load();
}

bool GameLogic::waitForMessage(GameMessage& msg)
{
    std::unique_lock<std::mutex> lock(mtx_);
    cvSendData_.wait(lock, [&] {
        return !messageQueue_.empty() || !running_.load();
    });

    if (!running_.load() && messageQueue_.empty()) return false;

    msg = messageQueue_.front();
    messageQueue_.pop();
    return true;
}

// ─── Private helpers ──────────────────────────────────────────────────────────

int GameLogic::chooseRandomLED()
{
    std::uniform_int_distribution<int> dist(0, numLEDs_ - 1);
    return dist(rng_);
}

void GameLogic::sendMessage(const std::string& type, int timeLeft)
{
    GameMessage msg;
    msg.type = type;
    msg.timeLeft = timeLeft;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        msg.score = score_;
        msg.round = round_;
        msg.activeLED = activeLED_;
        messageQueue_.push(msg);
    }

    cvSendData_.notify_one();
}
