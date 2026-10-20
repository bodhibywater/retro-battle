#include "retro/world/player_state.hpp"

#include "retro/world/movement.hpp"

namespace retro::world {

bool move_player(PlayerState& player, const TileMap& map, Direction direction) noexcept {
    player.facing = direction;
    const Position next = try_move(map, player.position, direction);
    if (next == player.position) {
        return false;
    }

    player.position = next;
    return true;
}

}  // namespace retro::world
