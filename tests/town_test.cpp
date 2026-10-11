#include <array>
#include <cstddef>
#include <queue>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "retro/world/direction.hpp"
#include "retro/world/movement.hpp"
#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

#include "sample_map.hpp"

using retro::world::Direction;
using retro::world::Position;
using retro::world::try_move;

TEST_CASE("sample town has a valid spawn and closed outer boundary") {
    const auto map = retro::client::make_sample_map();

    REQUIRE(map.width() == 40);
    REQUIRE(map.height() == 28);
    REQUIRE(map.is_walkable(Position{2, 2}));

    for (int x = 0; x < map.width(); ++x) {
        CHECK_FALSE(map.is_walkable(Position{x, 0}));
        CHECK_FALSE(map.is_walkable(Position{x, map.height() - 1}));
    }
    for (int y = 0; y < map.height(); ++y) {
        CHECK_FALSE(map.is_walkable(Position{0, y}));
        CHECK_FALSE(map.is_walkable(Position{map.width() - 1, y}));
    }
}

TEST_CASE("every walkable town tile is reachable from spawn") {
    const auto map = retro::client::make_sample_map();
    const Position spawn{2, 2};
    REQUIRE(map.is_walkable(spawn));

    const auto width = static_cast<std::size_t>(map.width());
    const auto height = static_cast<std::size_t>(map.height());
    std::vector<bool> visited(width * height, false);
    const auto index = [width](Position position) {
        return static_cast<std::size_t>(position.y) * width +
               static_cast<std::size_t>(position.x);
    };

    std::queue<Position> pending;
    pending.push(spawn);
    visited[index(spawn)] = true;

    constexpr std::array directions{
        Direction::North,
        Direction::South,
        Direction::West,
        Direction::East
    };

    while (!pending.empty()) {
        const Position position = pending.front();
        pending.pop();

        for (Direction direction : directions) {
            const Position next = try_move(map, position, direction);
            if (next != position && !visited[index(next)]) {
                visited[index(next)] = true;
                pending.push(next);
            }
        }
    }

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const Position position{x, y};
            if (map.is_walkable(position)) {
                INFO("Unreachable tile at (" << x << ", " << y << ")");
                CHECK(visited[index(position)]);
            }
        }
    }
}
