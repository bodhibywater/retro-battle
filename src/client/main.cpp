#include <iostream>

#include "retro/version.hpp"

int main() {
    std::cout << "Retro Battle client v" << retro::version() << '\n';
    return 0;
}