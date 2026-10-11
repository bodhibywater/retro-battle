
#include "retro/world/movement.hpp"

namespace retro::world {

Position try_move(
    const TileMap& map,
    Position current,
    Direction direction
) noexcept {

    if (!map.is_walkable(current)) {
        return current;
    }

    Position next = current;

    switch (direction) {
        case Direction::North:
            --next.y;
            break;

        case Direction::South:
            ++next.y;
            break;

        case Direction::East:
            ++next.x;
            break;

        case Direction::West:
            --next.x;
            break;
    }

    return map.is_walkable(next) ? next : current;
}

}  // namespace retro::world
