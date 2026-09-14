#include "GameLogic.h"
#include "LED.h"
#include "ForceSensor.h"
#include "SendData.h"
#include "Terminal.h"

#include <iostream>
#include <thread>
#include <vector>
#include <string>

int main()
{
    
    // GPIO BCM pin numbers for the 4 LEDs (RPi 5 → gpiochip15)
    std::vector<int> ledPins = {17, 27, 22, 5};

    // ADS1015 channels — LED 0 → A0, LED 1 → A1, LED 2 → A2, LED 3 → A3
    std::vector<int> sensorChannels = {0, 1, 2, 3};

    int numberOfRounds = 3;
    int roundDurationMs = 30000;   // 30 seconds per round
    int pauseBetweenRoundsMs = 3000;    // 3 second pause between rounds

    // LED stays on at most this long per round (decreasing each round)
    std::vector<int> ledTimePerRoundMs = {4000, 3000, 2000};

    // Raise threshold if detecting without a hit; lower it if missing real hits
    int sensorThreshold = 100;

    int adsAddress = 0x48;
    std::string i2cDevice = "/dev/i2c-1";

    try
    {
        LED led(ledPins);
        ForceSensor sensor(sensorChannels, sensorThreshold, adsAddress, i2cDevice);

        GameLogic gameLogic(
            led, sensor,
            static_cast<int>(ledPins.size()),
            numberOfRounds,
            roundDurationMs,
            pauseBetweenRoundsMs,
            ledTimePerRoundMs
        );

        Terminal terminal(gameLogic);
        SendData sendData(gameLogic, roundDurationMs);

        std::thread gameThread(&GameLogic::GameLogicThread, &gameLogic);
        std::thread termThread(&Terminal::TerminalThread,  &terminal);
        std::thread dataThread(&SendData::SendDataThread, &sendData);

        gameThread.join();
        termThread.join();
        dataThread.join();

        std::cout << "Program ended\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;

        
    }

    return 0;
}
