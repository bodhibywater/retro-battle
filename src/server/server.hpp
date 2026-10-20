#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "retro/net/protocol.hpp"

namespace retro::server {

class Session;

// All session callbacks run on a single io_context thread.
class Server {
public:
    Server(boost::asio::io_context& io, std::uint16_t port);

    void start();
    [[nodiscard]] std::uint16_t port() const;

private:
    friend class Session;

    void accept_next();

    void handle_message(
        const std::shared_ptr<Session>& session,
        const net::Message& message
    );

    void join(const std::shared_ptr<Session>& session);
    void disconnected(std::optional<net::PlayerId> id);

    void broadcast(
        const net::Message& message,
        net::PlayerId except = 0
    );

    boost::asio::ip::tcp::acceptor acceptor_;

    std::map<net::PlayerId, std::shared_ptr<Session>> players_;

    std::size_t pending_{};
    net::PlayerId next_id_{1};
};

}  // namespace retro::server
