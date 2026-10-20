#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <vector>

#include "retro/net/protocol.hpp"
#include "retro/world/player_state.hpp"
#include "retro/world/tile_map.hpp"

namespace retro::server {

// Authoritative positions and movement; no socket or raylib dependencies.
class WorldState {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto step_interval = std::chrono::milliseconds(150);

    WorldState(world::TileMap map, world::Position spawn);

    [[nodiscard]] std::optional<net::PlayerInfo> add(net::PlayerId id);
    void remove(net::PlayerId id);
    [[nodiscard]] std::vector<net::PlayerInfo> snapshot() const;

    // Returns a state update only when position or facing changes.
    [[nodiscard]] std::optional<net::PlayerInfo> move(
        net::PlayerId id,
        world::Direction direction,
        Clock::time_point now
    );

private:
    struct Player {
        world::PlayerState state;
        std::optional<Clock::time_point> last_attempt;
    };

    world::TileMap map_;
    world::Position spawn_;
    std::map<net::PlayerId, Player> players_;
};

}  // namespace retro::server
