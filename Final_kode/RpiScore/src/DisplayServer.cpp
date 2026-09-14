#include "DisplayServer.hpp"
#include <iostream>

DisplayServer::DisplayServer(int port)
    : port_(port) {}

DisplayServer::~DisplayServer()
{
    if (serverThread_.joinable())
        serverThread_.join();
}

void DisplayServer::sendMessage(const std::string & message)
{
    std::lock_guard<std::mutex> lock(wsMutex_);
    for (auto & [k, v] : m_registry)
    {
        v->send_message(rws::final_frame, rws::opcode_t::text_frame, message);
    }
}

void DisplayServer::sendJson(const std::string & json)
{
    sendMessage(json);
}

auto DisplayServer::createRouter() -> std::unique_ptr<router_t> 
{
    auto router = std::make_unique<router_t>(); // hukommelses management - undgår memory leak

    router->http_get("/chat", [this](restinio::request_handle_t req, // /chat : routing
                                     restinio::router::route_params_t)
    {
        if (restinio::http_connection_header_t::upgrade == req->header().connection())
        {
            auto ws = rws::upgrade<traits_t>( //opgraderer forbindelsen til websocket (handshake)
                *req,
                rws::activation_t::immediate,
                [this](rws::ws_handle_t wsh, rws::message_handle_t msg)
                {
                    if (msg->opcode() == rws::opcode_t::text_frame ||
                        msg->opcode() == rws::opcode_t::binary_frame ||
                        msg->opcode() == rws::opcode_t::continuation_frame)
                    {
                        wsh->send_message(*msg);
                    }
                    else if (msg->opcode() == rws::opcode_t::ping_frame)
                    {
                        auto resp = *msg;
                        resp.set_opcode(rws::opcode_t::pong_frame);
                        wsh->send_message(resp);
                    }
                    else if (msg->opcode() == rws::opcode_t::connection_close_frame)
                    {
                        std::lock_guard<std::mutex> lock(wsMutex_);
                        m_registry.erase(wsh->connection_id());
                    }
                });

            {
                std::lock_guard<std::mutex> lock(wsMutex_);
                m_registry.emplace(ws->connection_id(), ws); // bookkeeping - husker browseren
            }

            std::cout << "[DisplayServer] Browser connected.\n";
            return restinio::request_accepted();
        }

        return restinio::request_rejected();
    });

    return router; //overdrager unique_ptr til restinio - nu er det restinios ansvar at destruere når serveren lukker (oprydning)
}

void DisplayServer::start()
{
    serverThread_ = std::thread([this]() // opretter en ny OS-tråd der kører lambdaen
    {
        restinio::run( 
            restinio::on_thread_pool<traits_t>(1)
            .port(port_)
            .request_handler(createRouter())
        );
    });

    std::cout << "[DisplayServer] Listening on port " << port_ << "\n";
}