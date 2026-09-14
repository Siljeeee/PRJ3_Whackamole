#include "ReceiveData.hpp"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json_dto/pub.hpp>

ReceiveData::ReceiveData(const std::string & serverIp)
    : serverIp_(serverIp), sock_(-1) {}

ReceiveData::~ReceiveData()
{
    if (sock_ >= 0)
        close(sock_);
}

bool ReceiveData::connect()
{
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0)
    {
        std::cerr << "[ReceiveData] Failed to create socket.\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(8080);

    if (inet_pton(AF_INET, serverIp_.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "[ReceiveData] Invalid IP: " << serverIp_ << "\n";
        return false;
    }

    if (::connect(sock_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "[ReceiveData] Connection failed. Is the server running?\n";
        return false;
    }

    std::string handshake =
        "GET / HTTP/1.1\r\n"
        "Host: " + serverIp_ + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    send(sock_, handshake.c_str(), handshake.size(), 0);

    char buffer[1024] = {};
    recv(sock_, buffer, sizeof(buffer) - 1, 0);

    if (std::string(buffer).find("101") == std::string::npos)
    {
        std::cerr << "[ReceiveData] WebSocket upgrade failed.\n";
        return false;
    }

    std::cout << "[ReceiveData] WebSocket connected.\n";
    return true;
}

void ReceiveData::parseDisplayData()
{
    sendFrame("displayOn");
}

bool ReceiveData::parseData(GameState_t & state)
{
    while (true)
    {
        std::vector<uint8_t> frame(4096);
        ssize_t bytesRead = recv(sock_, frame.data(), frame.size(), 0);

        if (bytesRead <= 0)
        {
            std::cout << "[ReceiveData] Server closed connection.\n";
            return false;
        }

        frame.resize(bytesRead);
        std::string payload = extractPayload(frame);

        if (payload.empty())
            continue;

        try
        {
            state = json_dto::from_json<GameState_t>(payload);
            return true;
        }
        catch (const std::exception & e)
        {
            std::cerr << "[ReceiveData] JSON parse error: " << e.what() << "\n";
            continue;
        }
    }
}

void ReceiveData::sendDisplayUpdated()
{
    sendFrame("displayUpdated");
}

void ReceiveData::sendFrame(const std::string & message)
{
    std::vector<uint8_t> frame;
    frame.push_back(0x81);
    frame.push_back(0x80 | static_cast<uint8_t>(message.size()));

    uint8_t mask[4] = {0x37, 0xfa, 0x21, 0x3d};
    for (int i = 0; i < 4; i++)
        frame.push_back(mask[i]);

    for (size_t i = 0; i < message.size(); i++)
        frame.push_back(message[i] ^ mask[i % 4]);

    send(sock_, frame.data(), frame.size(), 0);
}

std::string ReceiveData::extractPayload(const std::vector<uint8_t> & frame)
{
    if (frame.size() < 2) return "";

    bool   masked = (frame[1] & 0x80) != 0;
    size_t len    = frame[1] & 0x7F;
    size_t offset = 2;

    if (len == 126)
    {
        if (frame.size() < 4) return "";
        len    = (frame[2] << 8) | frame[3];
        offset = 4;
    }

    std::string payload;
    if (masked)
    {
        if (frame.size() < offset + 4) return "";
        uint8_t mask[4] = {frame[offset], frame[offset+1], frame[offset+2], frame[offset+3]};
        offset += 4;
        for (size_t i = 0; i < len && (offset + i) < frame.size(); i++)
            payload += (char)(frame[offset + i] ^ mask[i % 4]);
    }
    else
    {
        for (size_t i = 0; i < len && (offset + i) < frame.size(); i++)
            payload += (char)frame[offset + i];
    }

    return payload;
}
