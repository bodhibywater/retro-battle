
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "retro/world/tile_map.hpp"

namespace retro::client {

inline world::TileMap make_sample_map() {
    std::vector<std::string> rows(
        12, std::string(20, '.')
    );

    // Border walls.
    for (int x = 0; x < 20; ++x) {
        rows[0][static_cast<std::size_t>(x)] = '#';
        rows[11][static_cast<std::size_t>(x)] = '#';
    }

    for (auto& row : rows) {
        row.front() = '#';
        row.back() = '#';
    }

    // A few obstacles.
    for (int x = 4; x <= 6; ++x) {
        rows[4][static_cast<std::size_t>(x)] = '#';
    }

    for (int y = 7; y <= 9; ++y) {
        rows[static_cast<std::size_t>(y)][13] = '#';
    }

    return world::TileMap(std::move(rows));
}

}  // namespace retro::client
