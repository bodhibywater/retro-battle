
#pragma once

namespace retro::world {

struct Position {
    int x{};
    int y{};

    bool operator==(const Position&) const = default;
};

}  // namespace retro::world
