#include <chrono>
#include <set>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "world_state.hpp"
#include "retro/world/sample_map.hpp"

using retro::server::WorldState;
using retro::world::Direction;
using retro::world::Position;
using retro::world::TileMap;

TEST_CASE("players receive unique walkable spawns") {
    WorldState world(
        retro::world::make_sample_map(),
        retro::world::sample_spawn
    );

    std::set<std::pair<int, int>> occupied;

    for (retro::net::PlayerId id = 1; id <= 8; ++id) {
        const auto player = world.add(id);
        REQUIRE(player.has_value());

        CHECK(player->id == id);
        CHECK(occupied.insert({
            player->position.x,
            player->position.y
        }).second);
    }

    CHECK(world.snapshot().size() == 8);
    CHECK_FALSE(world.add(9));
    CHECK_FALSE(world.add(1));
    CHECK_FALSE(world.add(0));

    world.remove(3);

    const auto replacement = world.add(9);
    REQUIRE(replacement.has_value());
    CHECK(replacement->id == 9);
    CHECK(world.snapshot().size() == 8);
}

TEST_CASE("movement obeys timing and player collision") {
    WorldState world(
        TileMap({
            "#####",
            "#...#",
            "#...#",
            "#####"
        }),
        {1, 1}
    );

    const auto alice = world.add(1);
    const auto bob = world.add(2);

    REQUIRE(alice.has_value());
    REQUIRE(bob.has_value());

    CHECK(alice->position == Position{1, 1});
    CHECK(bob->position == Position{2, 1});

    const auto start = WorldState::Clock::time_point{};

    // First step is accepted immediately.
    auto update = world.move(1, Direction::South, start);

    REQUIRE(update.has_value());
    CHECK(update->position == Position{1, 2});

    // A second step before the interval is rejected.
    CHECK_FALSE(world.move(
        1, Direction::East,
        start + std::chrono::milliseconds(149)
    ));

    // Exactly at the interval, movement is accepted.
    update = world.move(
        1,
        Direction::East,
        start + std::chrono::milliseconds(150)
    );

    REQUIRE(update.has_value());
    CHECK(update->position == Position{2, 2});
    CHECK(update->facing == Direction::East);

    // Bob occupies (2,1), blocking movement north.
    // Alice turns north but stays at (2,2).
    update = world.move(
        1,
        Direction::North,
        start + std::chrono::milliseconds(300)
    );

    REQUIRE(update.has_value());
    CHECK(update->position == Position{2, 2});
    CHECK(update->facing == Direction::North);

    // Repeating an unchanged blocked move produces no update.
    CHECK_FALSE(world.move(
        1,
        Direction::North,
        start + std::chrono::milliseconds(450)
    ));

    // Removing Bob makes the destination available again.
    world.remove(2);

    update = world.move(
        1,
        Direction::North,
        start + std::chrono::milliseconds(600)
    );

    REQUIRE(update.has_value());
    CHECK(update->position == Position{2, 1});

    CHECK_FALSE(world.move(
        999,
        Direction::South,
        start + std::chrono::milliseconds(1000)
    ));
}

TEST_CASE("new players receive current authoritative positions") {
    WorldState world(
        TileMap({
            "#####",
            "#...#",
            "#...#",
            "#####"
        }),
        {1, 1}
    );

    REQUIRE(world.add(1).has_value());

    const auto update = world.move(
        1,
        Direction::South,
        WorldState::Clock::time_point{}
    );

    REQUIRE(update.has_value());
    REQUIRE(world.add(2).has_value());

    const auto snapshot = world.snapshot();

    REQUIRE(snapshot.size() == 2);

    CHECK(snapshot[0].id == 1);
    CHECK(snapshot[0].position == Position{1, 2});

    CHECK(snapshot[1].id == 2);
    CHECK(snapshot[1].position == Position{1, 1});
}
