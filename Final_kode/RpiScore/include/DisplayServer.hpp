#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <restinio/core.hpp>
#include <restinio/router/express.hpp>
#include <restinio/websocket/websocket.hpp>

namespace rws = restinio::websocket::basic;
using router_t = restinio::router::express_router_t<>;
using ws_registry_t = std::map<std::uint64_t, rws::ws_handle_t>;

using traits_t = restinio::traits_t<
    restinio::asio_timer_manager_t,
    restinio::single_threaded_ostream_logger_t,
    router_t
>;

class DisplayServer
{
public:
    //explicit DisplayServer(int port = 9090); // til test på en RPi
    explicit DisplayServer(int port = 8080); // udkommenter når test op to RPi
    ~DisplayServer();

    void start();
    void sendJson(const std::string & json);

private:
    int              port_;
    std::thread      serverThread_;
    std::mutex       wsMutex_;
    ws_registry_t    m_registry;

    auto createRouter() -> std::unique_ptr<router_t>;
    void sendMessage(const std::string & message);
};