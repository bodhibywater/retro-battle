#include "retro/net/protocol.hpp"

#include <initializer_list>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <nlohmann/json.hpp>

namespace retro::net {
namespace {

using Json = nlohmann::json;

bool exact_fields(
    const Json& value,
    std::initializer_list<const char*> fields
) {
    if (!value.is_object() || value.size() != fields.size()) {
        return false;
    }

    for (const char* field : fields) {
        if (!value.contains(field)) {
            return false;
        }
    }
    return true;
}

std::string direction_name(world::Direction direction) {
    switch (direction) {
        case world::Direction::North: return "north";
        case world::Direction::South: return "south";
        case world::Direction::East: return "east";
        case world::Direction::West: return "west";
    }
    throw std::invalid_argument("Invalid direction");
}

world::Direction read_direction(const Json& value) {
    if (!value.is_string()) {
        throw std::invalid_argument("Direction must be a string");
    }

    const auto name = value.get<std::string>();
    if (name == "north") return world::Direction::North;
    if (name == "south") return world::Direction::South;
    if (name == "east") return world::Direction::East;
    if (name == "west") return world::Direction::West;

    throw std::invalid_argument("Unknown direction");
}

PlayerId read_id(const Json& value) {
    if (!value.is_number_unsigned()) {
        throw std::invalid_argument("Invalid player ID");
    }

    const auto id = value.get<std::uint64_t>();
    if (id == 0 || id > std::numeric_limits<PlayerId>::max()) {
        throw std::invalid_argument("Player ID out of range");
    }
    return static_cast<PlayerId>(id);
}

int read_coordinate(const Json& value) {
    if (!value.is_number_unsigned()) {
        throw std::invalid_argument("Invalid coordinate");
    }

    // A conservative wire-format bound, not the actual map bounds.
    const auto number = value.get<std::uint64_t>();
    if (number > 4095) {
        throw std::invalid_argument("Coordinate out of range");
    }
    return static_cast<int>(number);
}

Json player_json(const PlayerInfo& player) {
    return {
        {"id", player.id},
        {"x", player.position.x},
        {"y", player.position.y},
        {"facing", direction_name(player.facing)}
    };
}

PlayerInfo read_player(const Json& value) {
    if (!exact_fields(value, {"id", "x", "y", "facing"})) {
        throw std::invalid_argument("Invalid player fields");
    }

    return PlayerInfo{
        read_id(value.at("id")),
        {
            read_coordinate(value.at("x")),
            read_coordinate(value.at("y"))
        },
        read_direction(value.at("facing"))
    };
}

Json message_json(const Hello&) {
    return {
        {"type", "hello"},
        {"version", protocol_version}
    };
}

Json message_json(const Move& message) {
    return {
        {"type", "move"},
        {"direction", direction_name(message.direction)}
    };
}

Json message_json(const Welcome& message) {
    Json players = Json::array();
    for (const auto& player : message.players) {
        players.push_back(player_json(player));
    }

    return {
        {"type", "welcome"},
        {"player_id", message.player_id},
        {"players", std::move(players)}
    };
}

Json message_json(const PlayerJoined& message) {
    return {
        {"type", "player_joined"},
        {"player", player_json(message.player)}
    };
}

Json message_json(const PlayerMoved& message) {
    return {
        {"type", "player_moved"},
        {"player", player_json(message.player)}
    };
}

Json message_json(const PlayerLeft& message) {
    return {
        {"type", "player_left"},
        {"player_id", message.player_id}
    };
}

Json message_json(const Error& message) {
    return {
        {"type", "error"},
        {"reason", message.reason}
    };
}

Message parse_message(const Json& value) {
    if (!value.is_object() ||
        !value.contains("type") ||
        !value.at("type").is_string()) {
        throw std::invalid_argument("Missing message type");
    }

    const auto type = value.at("type").get<std::string>();

    if (type == "hello") {
        if (!exact_fields(value, {"type", "version"}) ||
            !value.at("version").is_number_unsigned() ||
            value.at("version").get<std::uint64_t>() != protocol_version) {
            throw std::invalid_argument("Unsupported protocol version");
        }
        return Hello{};
    }

    if (type == "move") {
        if (!exact_fields(value, {"type", "direction"})) {
            throw std::invalid_argument("Invalid move fields");
        }
        return Move{read_direction(value.at("direction"))};
    }

    if (type == "welcome") {
        if (!exact_fields(value, {"type", "player_id", "players"})) {
            throw std::invalid_argument("Invalid welcome fields");
        }

        const PlayerId own_id = read_id(value.at("player_id"));
        const Json& entries = value.at("players");

        if (!entries.is_array() ||
            entries.empty() ||
            entries.size() > max_players) {
            throw std::invalid_argument("Invalid player list");
        }

        std::vector<PlayerInfo> players;
        std::set<PlayerId> ids;

        for (const auto& entry : entries) {
            auto player = read_player(entry);
            if (!ids.insert(player.id).second) {
                throw std::invalid_argument("Duplicate player ID");
            }
            players.push_back(player);
        }

        if (!ids.contains(own_id)) {
            throw std::invalid_argument("Own player missing");
        }

        return Welcome{own_id, std::move(players)};
    }

    if (type == "player_joined") {
        if (!exact_fields(value, {"type", "player"})) {
            throw std::invalid_argument("Invalid join fields");
        }
        return PlayerJoined{read_player(value.at("player"))};
    }

    if (type == "player_moved") {
        if (!exact_fields(value, {"type", "player"})) {
            throw std::invalid_argument("Invalid movement fields");
        }
        return PlayerMoved{read_player(value.at("player"))};
    }

    if (type == "player_left") {
        if (!exact_fields(value, {"type", "player_id"})) {
            throw std::invalid_argument("Invalid leave fields");
        }
        return PlayerLeft{read_id(value.at("player_id"))};
    }

    if (type == "error") {
        if (!exact_fields(value, {"type", "reason"}) ||
            !value.at("reason").is_string()) {
            throw std::invalid_argument("Invalid error fields");
        }

        const auto reason = value.at("reason").get<std::string>();
        if (reason.empty() || reason.size() > 128) {
            throw std::invalid_argument("Invalid error reason");
        }
        return Error{reason};
    }

    throw std::invalid_argument("Unknown message type");
}

}  // namespace

std::string serialize(const Message& message) {
    const std::string payload = std::visit(
        [](const auto& value) {
            return message_json(value).dump();
        },
        message
    );

    if (payload.empty() || payload.size() > max_payload_size) {
        throw std::length_error("Network message too large");
    }
    return payload;
}

std::optional<Message> deserialize(std::string_view payload) {
    if (payload.empty() || payload.size() > max_payload_size) {
        return std::nullopt;
    }

    try {
        const auto value = Json::parse(payload.begin(), payload.end());
        return parse_message(value);
    } catch (const Json::exception&) {
        return std::nullopt;
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
}

}  // namespace retro::net
