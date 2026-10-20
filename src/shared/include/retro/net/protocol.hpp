#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "retro/world/direction.hpp"
#include "retro/world/position.hpp"

namespace retro::net {

inline constexpr std::uint32_t protocol_version = 1;
inline constexpr std::size_t max_payload_size = 4096;
inline constexpr std::size_t max_players = 8;

using PlayerId = std::uint32_t;

// A player's authoritative state as transmitted over the network.
struct PlayerInfo {
    PlayerId id{};
    world::Position position{};
    world::Direction facing{world::Direction::South};

    bool operator==(const PlayerInfo&) const = default;
};

struct Hello {
    bool operator==(const Hello&) const = default;
};

struct Move {
    world::Direction direction{};
    bool operator==(const Move&) const = default;
};

struct Welcome {
    PlayerId player_id{};
    std::vector<PlayerInfo> players;
    bool operator==(const Welcome&) const = default;
};

struct PlayerJoined {
    PlayerInfo player;
    bool operator==(const PlayerJoined&) const = default;
};

struct PlayerMoved {
    PlayerInfo player;
    bool operator==(const PlayerMoved&) const = default;
};

struct PlayerLeft {
    PlayerId player_id{};
    bool operator==(const PlayerLeft&) const = default;
};

struct Error {
    std::string reason;
    bool operator==(const Error&) const = default;
};

using Message = std::variant<
    Hello, Move, Welcome, PlayerJoined,
    PlayerMoved, PlayerLeft, Error
>;

// JSON payload only; TCP framing is handled separately.
// serialize() throws if its output exceeds the payload limit.
// deserialize() returns nullopt for malformed or invalid input.
[[nodiscard]] std::string serialize(const Message& message);
[[nodiscard]] std::optional<Message> deserialize(std::string_view payload);

}  // namespace retro::net
