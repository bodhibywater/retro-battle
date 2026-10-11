#include <iostream>

#include <boost/asio/io_context.hpp>

#include "retro/version.hpp"

int main() {
    boost::asio::io_context io_context;

    std::cout
        << "Retro Battle server v"
        << retro::version()
        << '\n';

    std::cout
        << "Networking runtime initialised"
        << '\n';

    io_context.run();

    return 0;
}