#include "server.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>

#include "retro/net/framing.hpp"
#include "retro/world/sample_map.hpp"

namespace retro::server {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost::system::error_code;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(Server& server, tcp::socket socket)
        : server_(server),
          socket_(std::move(socket)),
          timer_(socket_.get_executor()) {}

    void start() {
        // Don't allow idle, unauthenticated connections indefinitely.
        timer_.expires_after(std::chrono::seconds(5));

        timer_.async_wait(
            [self = shared_from_this()](const error_code& ec) {
                if (!ec && !self->id_) {
                    self->close();
                }
            }
        );

        read_next();
    }

    [[nodiscard]] std::optional<net::PlayerId> id() const {
        return id_;
    }

    void joined(net::PlayerId id) {
        id_ = id;
        timer_.cancel();
    }

    void send(const net::Message& message) {
        if (closed_ || closing_) {
            return;
        }

        const bool writing = !writes_.empty();

        writes_.push_back(
            net::frame_message(net::serialize(message))
        );

        if (!writing) {
            write_next();
        }
    }

    void reject(const std::string& reason) {
        if (closed_ || closing_) {
            return;
        }

        send(net::Error{reason});
        closing_ = true;
    }

private:
    void read_next() {
        auto self = shared_from_this();

        socket_.async_read_some(
            asio::buffer(read_buffer_),
            [self](const error_code& ec, std::size_t size) {
                if (ec) {
                    self->close();
                    return;
                }

                try {
                    const auto frames = self->decoder_.feed(
                        std::string_view(
                            self->read_buffer_.data(),
                            size
                        )
                    );

                    for (const auto& payload : frames) {
                        const auto message = net::deserialize(payload);

                        if (!message) {
                            self->reject("Invalid message");
                            break;
                        }

                        self->server_.handle_message(self, *message);

                        if (self->closing_ || self->closed_) {
                            break;
                        }
                    }
                } catch (const std::invalid_argument&) {
                    self->reject("Invalid frame");
                }

                if (!self->closing_ && !self->closed_) {
                    self->read_next();
                }
            }
        );
    }

    void write_next() {
        auto self = shared_from_this();

        // writes_.front() stays alive until this write completes.
        asio::async_write(
            socket_,
            asio::buffer(writes_.front()),
            [self](const error_code& ec, std::size_t) {
                if (ec) {
                    self->close();
                    return;
                }

                self->writes_.pop_front();

                if (!self->writes_.empty()) {
                    self->write_next();
                } else if (self->closing_) {
                    self->close();
                }
            }
        );
    }

    void close() {
        if (closed_) {
            return;
        }

        closed_ = true;
        timer_.cancel();

        error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);

        server_.disconnected(id_);
    }

    Server& server_;
    tcp::socket socket_;
    asio::steady_timer timer_;

    net::FrameDecoder decoder_;
    std::array<char, 4096> read_buffer_{};
    std::deque<std::string> writes_;

    std::optional<net::PlayerId> id_;

    bool closing_{};
    bool closed_{};
};

Server::Server(asio::io_context& io, std::uint16_t port)
    : acceptor_(
        io,
        tcp::endpoint(
            asio::ip::make_address_v4("127.0.0.1"),
            port
        )
    ),
      world_(world::make_sample_map(), world::sample_spawn) {}

void Server::start() {
    std::cout
        << "Listening on 127.0.0.1:"
        << port()
        << std::endl;

    accept_next();
}

std::uint16_t Server::port() const {
    return acceptor_.local_endpoint().port();
}

void Server::accept_next() {
    acceptor_.async_accept(
        [this](const error_code& ec, tcp::socket socket) {
            if (!ec) {
                if (players_.size() + pending_ >= net::max_players) {
                    // Refuse excess connections without blocking the
                    // event loop. Keep socket and buffer alive until sent.
                    auto peer = std::make_shared<tcp::socket>(
                        std::move(socket)
                    );

                    auto frame = std::make_shared<std::string>(
                        net::frame_message(
                            net::serialize(
                                net::Error{"Server full"}
                            )
                        )
                    );

                    asio::async_write(
                        *peer,
                        asio::buffer(*frame),
                        [peer, frame](
                            const error_code&,
                            std::size_t
                        ) {}
                    );
                } else {
                    ++pending_;

                    std::make_shared<Session>(
                        *this,
                        std::move(socket)
                    )->start();
                }
            } else {
                std::cerr
                    << "Accept error: "
                    << ec.message()
                    << '\n';
            }

            if (acceptor_.is_open()) {
                accept_next();
            }
        }
    );
}

void Server::handle_message(
    const std::shared_ptr<Session>& session,
    const net::Message& message
) {
    if (!session->id()) {
        if (std::holds_alternative<net::Hello>(message)) {
            join(session);
        } else {
            session->reject("Expected hello");
        }

        return;
    }

    if (const auto* move = std::get_if<net::Move>(&message)) {
        const auto updated = world_.move(
            *session->id(),
            move->direction,
            WorldState::Clock::now()
        );

        if (updated) {
            // Send to all clients, including the player who moved.
            broadcast(net::PlayerMoved{*updated});
        }
        return;
    }

    session->reject("Unexpected message");
}

void Server::join(const std::shared_ptr<Session>& session) {
    if (next_id_ == 0) {
        session->reject("Server ID limit reached");
        return;
    }

    const net::PlayerId id = next_id_;
    const auto newcomer = world_.add(id);

    if (!newcomer) {
        session->reject("No available spawn");
        return;
    }

    ++next_id_;

    --pending_;
    session->joined(id);
    players_.emplace(id, session);

    session->send(net::Welcome{id, world_.snapshot()});
    broadcast(net::PlayerJoined{*newcomer}, id);

    std::cout
        << "Player " << id
        << " joined (" << players_.size()
        << " online)\n";
}

void Server::disconnected(std::optional<net::PlayerId> id) {
    if (!id) {
        --pending_;
        return;
    }

    world_.remove(*id);
    players_.erase(*id);
    broadcast(net::PlayerLeft{*id});

    std::cout
        << "Player " << *id
        << " left (" << players_.size()
        << " online)\n";
}

void Server::broadcast(
    const net::Message& message,
    net::PlayerId except
) {
    for (const auto& [id, session] : players_) {
        if (id != except) {
            session->send(message);
        }
    }
}

}  // namespace retro::server
