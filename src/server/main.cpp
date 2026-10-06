#include <iostream>

#include "retro/version.hpp"

int main() {
    std::cout << "Retro Battle server v" << retro::version() << '\n';
    return 0;
}