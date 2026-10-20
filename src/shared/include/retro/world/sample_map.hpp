#pragma once

#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

namespace retro::world {

// Single-player spawn and initial networking spawn. Server-owned spawn
// allocation can be introduced when multiple clients are supported.
inline constexpr Position sample_spawn{2, 2};

[[nodiscard]] TileMap make_sample_map();

}  // namespace retro::world
