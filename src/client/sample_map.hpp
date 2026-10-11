
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "retro/world/tile_map.hpp"

namespace retro::client {

inline world::TileMap make_sample_map() {
    constexpr int width = 40;
    constexpr int height = 28;

    std::vector<std::string> rows(
        height, std::string(width, '.')
    );

    // Outer boundary.
    for (int x = 0; x < width; ++x) {
        rows[0][static_cast<std::size_t>(x)] = '#';
        rows[height - 1][static_cast<std::size_t>(x)] = '#';
    }

    for (auto& row : rows) {
        row.front() = '#';
        row.back() = '#';
    }

    // Scattered obstacles to test camera movement
    // and collision across the larger world.
    for (int x = 5; x <= 11; ++x) {
        rows[5][static_cast<std::size_t>(x)] = '#';
    }

    for (int y = 9; y <= 15; ++y) {
        rows[static_cast<std::size_t>(y)][22] = '#';
    }

    for (int x = 28; x <= 35; ++x) {
        rows[19][static_cast<std::size_t>(x)] = '#';
    }

    for (int y = 18; y <= 23; ++y) {
        rows[static_cast<std::size_t>(y)][8] = '#';
    }

    return world::TileMap(std::move(rows));
}

}  // namespace retro::client
