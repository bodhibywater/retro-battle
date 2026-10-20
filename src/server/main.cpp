#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>

#include <boost/asio/io_context.hpp>

#include "retro/version.hpp"
#include "server.hpp"

int main(int argc, char* argv[]) {
    std::uint16_t port = 7777;

    if (argc != 1) {
        if (argc != 3 ||
            std::string_view(argv[1]) != "--port") {
            std::cerr << "Usage: retro_server [--port 0..65535]\n";
            return 2;
        }

        const std::string_view value(argv[2]);
        unsigned int parsed{};

        const auto [end, ec] = std::from_chars(
            value.data(),
            value.data() + value.size(),
            parsed
        );

        if (ec != std::errc{} ||
            end != value.data() + value.size() ||
            parsed > 65535) {
            std::cerr << "Invalid TCP port\n";
            return 2;
        }

        port = static_cast<std::uint16_t>(parsed);
    }

    try {
        boost::asio::io_context io;

        retro::server::Server server(io, port);

        std::cout
            << "Retro Battle server v"
            << retro::version()
            << '\n';

        server.start();
        io.run();
    } catch (const std::exception& error) {
        std::cerr
            << "Server error: "
            << error.what()
            << '\n';
        return 1;
    }
}
