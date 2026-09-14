#include "SendData.h"
#include "GameLogic.h"

#include <iostream>
#include <algorithm>
#include <json_dto/pub.hpp>

SendData::SendData(GameLogic& gameLogic, int roundDurationMs)
    : gameLogic_(gameLogic), roundDurationMs_(roundDurationMs)
{
}

SendData::~SendData()
{
    if (serverThread_.joinable())
        serverThread_.join();
}

// ─── Public ──────────────────────────────────────────────────────────────────

void SendData::SendDataThread()
{
    startServer();

    GameMessage msg;
    while (gameLogic_.waitForMessage(msg))
    {
        sendJSON(msg);
    }

    std::cout << "[SendData] stopped.\n";
}

// ─── Private ─────────────────────────────────────────────────────────────────

void SendData::startServer()
{
    serverThread_ = std::thread([this]()
    {
        restinio::run(
            restinio::on_thread_pool<traits_t>(1)
            .port(8080)
            .request_handler(createRouter())
        );
    });

    std::cout << "[SendData] WebSocket server listening on port 8080...\n";
}

auto SendData::createRouter() -> std::unique_ptr<router_t>
{
    auto router = std::make_unique<router_t>();

    router->http_get("/", [this](restinio::request_handle_t req,
                                  restinio::router::route_params_t)
    {
        if (restinio::http_connection_header_t::upgrade == req->header().connection())
        {
            auto ws = rws::upgrade<traits_t>(
                *req,
                rws::activation_t::immediate,
                [this](rws::ws_handle_t wsh, rws::message_handle_t msg)
                {
                    if (msg->opcode() == rws::opcode_t::ping_frame)
                    {
                        auto resp = *msg;
                        resp.set_opcode(rws::opcode_t::pong_frame);
                        wsh->send_message(resp);
                    }
                    else if (msg->opcode() == rws::opcode_t::connection_close_frame)
                    {
                        std::lock_guard<std::mutex> lock(wsMutex_);
                        wsRegistry_.erase(wsh->connection_id());
                        std::cout << "[SendData] RpiScore disconnected.\n";
                    }
                    // text frames (displayOn, displayUpdated) ignoreres
                });

            {
                std::lock_guard<std::mutex> lock(wsMutex_);
                wsRegistry_.emplace(ws->connection_id(), ws);
            }

            std::cout << "[SendData] RpiScore connected.\n";
            return restinio::request_accepted();
        }

        return restinio::request_rejected();
    });

    return router;
}

void SendData::broadcast(const std::string& json)
{
    std::lock_guard<std::mutex> lock(wsMutex_);
    for (auto& [id, ws] : wsRegistry_)
    {
        ws->send_message(rws::final_frame, rws::opcode_t::text_frame, json);
    }
}

void SendData::sendJSON(const GameMessage& msg)
{
    using namespace std::chrono;

    GameState_t state;
    state.score = msg.score;
    state.round = msg.round;

    if (msg.type == "start_game")
    {
        state.gameState = "active";
    }
    else if (msg.type == "start_round")
    {
        state.gameState = "active";
        state.time = roundDurationMs_ / 1000;
        roundStarted_ = true;
        roundStartTime_ = steady_clock::now();
    }
    else if (msg.type == "timer_update")
    {
        state.gameState = "active";
        state.time = msg.timeLeft;
    }
    else if (msg.type == "led_active"   ||
             msg.type == "update_point" ||
             msg.type == "miss")
    {
        state.gameState = "active";
        if (roundStarted_)
        {
            auto elapsed = duration_cast<milliseconds>(
                steady_clock::now() - roundStartTime_
            ).count();
            state.time = std::max(0, (roundDurationMs_ - static_cast<int>(elapsed)) / 1000);
        }
    }
    else if (msg.type == "countdown")
    {
        state.gameState = "countdown";
        state.countdown = msg.timeLeft;
        roundStarted_ = false;
    }
    else if (msg.type == "game_over" || msg.type == "final_score")
    {
        state.gameState = "idle";
        roundStarted_ = false;
    }
    else
    {
        return;
    }

    broadcast(json_dto::to_json(state));
}
