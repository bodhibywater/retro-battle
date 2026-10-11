
#pragma once

#include "retro/world/direction.hpp"
#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

namespace retro::world {

[[nodiscard]] Position try_move(
    const TileMap& map,
    Position current,
    Direction direction
) noexcept;

}  // namespace retro::world
