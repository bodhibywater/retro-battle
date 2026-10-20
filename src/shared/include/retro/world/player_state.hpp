#pragma once

#include "retro/world/direction.hpp"
#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

namespace retro::world {

// Game state shared by server and client. Never depends on raylib or input.
struct PlayerState {
    Position position{};
    Direction facing{Direction::South};
};

// Applies a single tile step. Updates facing even if movement is blocked.
// Rate limiting and input validation belong to the authoritative server.
[[nodiscard]] bool move_player(
    PlayerState& player,
    const TileMap& map,
    Direction direction
) noexcept;

}  // namespace retro::world
