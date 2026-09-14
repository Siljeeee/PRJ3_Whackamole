#ifndef SENDDATA_H
#define SENDDATA_H

#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <chrono>

#include <restinio/core.hpp>
#include <restinio/router/express.hpp>
#include <restinio/websocket/websocket.hpp>

#include "GameState.hpp"
#include "GameLogic.h"

namespace rws = restinio::websocket::basic;
using router_t = restinio::router::express_router_t<>;
using ws_registry_t = std::map<std::uint64_t, rws::ws_handle_t>;

using traits_t = restinio::traits_t<
    restinio::asio_timer_manager_t,
    restinio::single_threaded_ostream_logger_t,
    router_t
>;


class SendData
{
public:
    explicit SendData(GameLogic& gameLogic, int roundDurationMs);
    ~SendData();
    void SendDataThread();

private:
    GameLogic& gameLogic_;
    int roundDurationMs_;
    bool roundStarted_ = false;
    std::chrono::steady_clock::time_point roundStartTime_;

    std::thread serverThread_;
    std::mutex wsMutex_;
    ws_registry_t wsRegistry_;

    auto createRouter() -> std::unique_ptr<router_t>;
    void startServer();
    void sendJSON(const GameMessage& msg);
    void broadcast(const std::string& json);
};

#endif
