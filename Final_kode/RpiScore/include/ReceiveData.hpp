#pragma once
#include <string>
#include <vector>
#include "GameState.hpp"

class ReceiveData
{
public:
    ReceiveData(const std::string & serverIp);
    ~ReceiveData();

    bool connect();
    void parseDisplayData();
    bool parseData(GameState_t & state);
    void sendDisplayUpdated();

private:
    std::string serverIp_;
    int         sock_;

    void        sendFrame(const std::string & message);
    std::string extractPayload(const std::vector<uint8_t> & frame);
};
