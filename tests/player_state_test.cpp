#include <catch2/catch_test_macros.hpp>

#include "retro/world/player_state.hpp"
#include "retro/world/sample_map.hpp"

using retro::world::Direction;
using retro::world::PlayerState;
using retro::world::Position;
using retro::world::TileMap;
using retro::world::move_player;

TEST_CASE("shared player state accepts valid movement") {
    const TileMap map({"...", ".#.", "..."});
    PlayerState player{{0, 1}, Direction::South};

    REQUIRE(move_player(player, map, Direction::North));
    CHECK(player.position == Position{0, 0});
    CHECK(player.facing == Direction::North);
}

TEST_CASE("blocked movement updates facing without changing position") {
    const TileMap map({"...", ".#.", "..."});
    PlayerState player{{0, 1}, Direction::South};

    CHECK_FALSE(move_player(player, map, Direction::East));
    CHECK(player.position == Position{0, 1});
    CHECK(player.facing == Direction::East);
}

TEST_CASE("shared sample map exposes a valid initial spawn") {
    const auto map = retro::world::make_sample_map();
    CHECK(map.is_walkable(retro::world::sample_spawn));
}
