#include <iostream>
#include <string_view>

#include "retro/version.hpp"

int main() {
    constexpr std::string_view expected_version{"0.1.0"};

    if (retro::version() != expected_version) {
        std::cerr << "Expected version " << expected_version
                  << ", got " << retro::version() << '\n';
        return 1;
    }

    return 0;
}