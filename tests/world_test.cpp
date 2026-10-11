
#include <stdexcept>
#include <vector>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "retro/world/movement.hpp"
#include "retro/world/tile_map.hpp"

using retro::world::Direction;
using retro::world::Position;
using retro::world::TileMap;
using retro::world::try_move;

TEST_CASE("map dimensions and bounds") {
    const TileMap map({
        "....",
        ".##.",
        "...."
    });

    REQUIRE(map.width() == 4);
    REQUIRE(map.height() == 3);

    REQUIRE(map.contains({0, 0}));
    REQUIRE(map.contains({3, 2}));

    REQUIRE_FALSE(map.contains({-1, 0}));
    REQUIRE_FALSE(map.contains({4, 1}));
    REQUIRE_FALSE(map.contains({0, 3}));
}

TEST_CASE("walkability respects obstacles and boundaries") {
    const TileMap map({
        "...",
        ".#.",
        "..."
    });

    REQUIRE(map.is_walkable({0, 0}));
    REQUIRE_FALSE(map.is_walkable({1, 1}));
    REQUIRE_FALSE(map.is_walkable({-1, 0}));
}

TEST_CASE("movement works in all four directions") {
    const TileMap map({
        "...",
        "...",
        "..."
    });

    const Position centre{1, 1};

    REQUIRE(try_move(map, centre, Direction::North) == Position{1, 0});
    REQUIRE(try_move(map, centre, Direction::South) == Position{1, 2});
    REQUIRE(try_move(map, centre, Direction::East) == Position{2, 1});
    REQUIRE(try_move(map, centre, Direction::West) == Position{0, 1});
}

TEST_CASE("movement cannot enter blocked tiles") {
    const TileMap map({
        "...",
        ".#.",
        "..."
    });

    REQUIRE(
        try_move(map, {0, 1}, Direction::East) == Position{0, 1}
    );
}

TEST_CASE("movement cannot leave the map") {
    const TileMap map({
        "..",
        ".."
    });

    REQUIRE(
        try_move(map, {0, 0}, Direction::North) == Position{0, 0}
    );

    REQUIRE(
        try_move(map, {0, 0}, Direction::West) == Position{0, 0}
    );
}

TEST_CASE("movement rejects an invalid starting tile") {
    const TileMap map({
        ".#",
        ".."
    });

    REQUIRE(
        try_move(map, {1, 0}, Direction::South) == Position{1, 0}
    );

    REQUIRE(
        try_move(map, {-1, 0}, Direction::East) == Position{-1, 0}
    );
}

TEST_CASE("invalid maps are rejected") {
    REQUIRE_THROWS_AS(
        TileMap(std::vector<std::string>{}),
        std::invalid_argument
    );

    REQUIRE_THROWS_AS(
        TileMap(std::vector<std::string>{"...", ".."}),
        std::invalid_argument
    );

    REQUIRE_THROWS_AS(
        TileMap(std::vector<std::string>{".?"}),
        std::invalid_argument
    );
}
