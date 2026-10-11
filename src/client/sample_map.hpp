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

    std::vector<std::string> rows(height, std::string(width, 'g'));

    const auto set_tile = [&rows](int x, int y, char tile) {
        rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = tile;
    };

    // Forest border and clusters. Routes remain accessible.
    for (int x = 0; x < width; ++x) {
        set_tile(x, 0, 't');
        set_tile(x, height - 1, 't');
    }
    for (int y = 0; y < height; ++y) {
        set_tile(0, y, 't');
        set_tile(width - 1, y, 't');
    }

    for (int x = 4; x <= 12; ++x) {
        for (int y = 3; y <= 4; ++y) {
            set_tile(x, y, 't');
        }
    }
    for (int x = 3; x <= 8; ++x) {
        for (int y = 19; y <= 21; ++y) {
            set_tile(x, y, 't');
        }
    }
    for (int x = 29; x <= 36; ++x) {
        for (int y = 21; y <= 24; ++y) {
            if (x == 29 || x == 36 || y == 21 || y == 24) {
                set_tile(x, y, 't');
            }
        }
    }

    // Entrance to the southeastern clearing; do not strand walkable tiles.
    set_tile(29, 22, 'g');

    // Pond in the southeast.
    for (int y = 17; y <= 20; ++y) {
        for (int x = 30; x <= 36; ++x) {
            set_tile(x, y, 'w');
        }
    }

    // Main crossroads and paths to the two houses.
    for (int x = 2; x <= 37; ++x) {
        set_tile(x, 14, 'p');
        set_tile(x, 15, 'p');
    }
    for (int y = 5; y <= 25; ++y) {
        set_tile(19, y, 'p');
        set_tile(20, y, 'p');
    }
    for (int y = 10; y <= 15; ++y) {
        set_tile(9, y, 'p');
        set_tile(10, y, 'p');
        set_tile(27, y, 'p');
        set_tile(28, y, 'p');
    }

    // Building footprints are impassable; 'h' marks the sprite origin.
    for (int origin_x : {7, 25}) {
        for (int y = 6; y < 10; ++y) {
            for (int x = origin_x; x < origin_x + 5; ++x) {
                set_tile(x, y, 'b');
            }
        }
        set_tile(origin_x, 6, 'h');
    }

    for (const auto& [x, y] : {std::pair{12, 17}, {15, 20}, {33, 9}, {23, 22}}) {
        set_tile(x, y, 'r');
    }
    for (const auto& [x, y] : {std::pair{5, 9}, {14, 11}, {24, 11},
                             {16, 23}, {25, 18}, {5, 24}}) {
        set_tile(x, y, 'f');
    }

    return world::TileMap(std::move(rows));
}

}  // namespace retro::client
