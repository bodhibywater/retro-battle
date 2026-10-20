#include "world_state.hpp"

#include <array>
#include <cstddef>
#include <queue>
#include <utility>
#include <vector>

#include "retro/world/movement.hpp"

namespace retro::server {
namespace {

net::PlayerInfo as_info(net::PlayerId id, const world::PlayerState& state) {
    return {id, state.position, state.facing};
}

}  // namespace

WorldState::WorldState(world::TileMap map, world::Position spawn)
    : map_(std::move(map)), spawn_(spawn) {}

std::optional<net::PlayerInfo> WorldState::add(net::PlayerId id) {
    if (id == 0 || players_.contains(id) ||
        players_.size() >= net::max_players || !map_.is_walkable(spawn_)) {
        return std::nullopt;
    }

    const auto width = static_cast<std::size_t>(map_.width());
    const auto height = static_cast<std::size_t>(map_.height());

    std::vector<bool> visited(width * height, false);

    const auto index = [width](world::Position p) {
        return static_cast<std::size_t>(p.y) * width +
               static_cast<std::size_t>(p.x);
    };

    std::queue<world::Position> remaining;
    remaining.push(spawn_);
    visited[index(spawn_)] = true;

    constexpr std::array directions{
        world::Direction::North,
        world::Direction::East,
        world::Direction::South,
        world::Direction::West
    };

    // Breadth-first search: select the nearest unoccupied walkable tile.
    while (!remaining.empty()) {
        const auto position = remaining.front();
        remaining.pop();

        bool occupied = false;

        for (const auto& [other_id, player] : players_) {
            if (player.state.position == position) {
                occupied = true;
                break;
            }
        }

        if (!occupied) {
            world::PlayerState state{
                position, world::Direction::South
            };

            players_.emplace(id, Player{state, std::nullopt});
            return as_info(id, state);
        }

        for (const auto direction : directions) {
            const auto next = world::try_move(
                map_, position, direction
            );

            if (next != position && !visited[index(next)]) {
                visited[index(next)] = true;
                remaining.push(next);
            }
        }
    }

    return std::nullopt;
}

void WorldState::remove(net::PlayerId id) {
    players_.erase(id);
}

std::vector<net::PlayerInfo> WorldState::snapshot() const {
    std::vector<net::PlayerInfo> result;
    result.reserve(players_.size());

    for (const auto& [id, player] : players_) {
        result.push_back(as_info(id, player.state));
    }

    return result;
}

std::optional<net::PlayerInfo> WorldState::move(
    net::PlayerId id,
    world::Direction direction,
    Clock::time_point now
) {
    const auto found = players_.find(id);

    if (found == players_.end()) {
        return std::nullopt;
    }

    auto& player = found->second;

    // The server controls movement speed.
    if (player.last_attempt &&
        now - *player.last_attempt < step_interval) {
        return std::nullopt;
    }

    const auto before = player.state;

    // Reuse the shared movement and map collision rules.
    (void)world::move_player(player.state, map_, direction);

    // Players cannot step into another player's occupied tile.
    for (const auto& [other_id, other] : players_) {
        if (other_id != id &&
            other.state.position == player.state.position) {
            player.state.position = before.position;
            break;
        }
    }

    player.last_attempt = now;

    // No update is needed when nothing actually changed.
    if (player.state.position == before.position &&
        player.state.facing == before.facing) {
        return std::nullopt;
    }

    return as_info(id, player.state);
}

}  // namespace retro::server
