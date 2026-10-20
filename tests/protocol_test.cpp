#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "retro/net/framing.hpp"
#include "retro/net/protocol.hpp"

using namespace retro::net;
using retro::world::Direction;

TEST_CASE("all protocol messages round-trip through JSON") {
    const PlayerInfo alice{1, {2, 2}, Direction::South};
    const PlayerInfo bob{2, {3, 4}, Direction::West};

    const std::vector<Message> messages{
        Hello{},
        Move{Direction::North},
        Move{Direction::South},
        Move{Direction::East},
        Move{Direction::West},
        Welcome{1, {alice, bob}},
        PlayerJoined{bob},
        PlayerMoved{alice},
        PlayerLeft{2},
        Error{"Server full"}
    };

    for (const auto& message : messages) {
        const auto decoded = deserialize(serialize(message));
        REQUIRE(decoded.has_value());
        CHECK(*decoded == message);
    }
}

TEST_CASE("hello requires the supported protocol version") {
    CHECK(deserialize(R"({"type":"hello","version":1})").has_value());
    CHECK_FALSE(deserialize(R"({"type":"hello","version":2})"));
    CHECK_FALSE(deserialize(R"({"type":"hello","version":"1"})"));
    CHECK_FALSE(deserialize(R"({"type":"hello"})"));
}

TEST_CASE("invalid JSON messages are rejected") {
    CHECK_FALSE(deserialize("not json"));
    CHECK_FALSE(deserialize("{}"));
    CHECK_FALSE(deserialize(R"({"type":"unknown"})"));

    CHECK_FALSE(deserialize(
        R"({"type":"move","direction":"diagonal"})"
    ));
    CHECK_FALSE(deserialize(
        R"({"type":"move","direction":42})"
    ));
    CHECK_FALSE(deserialize(
        R"({"type":"move","direction":"north","x":100})"
    ));
    CHECK_FALSE(deserialize(
        R"({"type":"player_left","player_id":0})"
    ));
    CHECK_FALSE(deserialize(
        R"({"type":"player_left","player_id":-1})"
    ));
    CHECK_FALSE(deserialize(
        R"({"type":"player_left","player_id":4294967296})"
    ));
}

TEST_CASE("welcome rejects inconsistent player lists") {
    const std::string player =
        R"({"id":1,"x":2,"y":2,"facing":"south"})";

    const std::string duplicate =
        R"({"type":"welcome","player_id":1,"players":[)" +
        player + "," + player + "]}";

    const std::string missing_own =
        R"({"type":"welcome","player_id":2,"players":[)" +
        player + "]}";

    CHECK_FALSE(deserialize(duplicate));
    CHECK_FALSE(deserialize(missing_own));
    CHECK_FALSE(deserialize(
        R"({"type":"player_joined","player":{"id":1,"x":5000,"y":2,"facing":"south"}})"
    ));
}

TEST_CASE("frame decoder handles a message split across reads") {
    const auto payload = serialize(Message{Move{Direction::East}});
    const auto framed = frame_message(payload);

    FrameDecoder decoder;
    std::vector<std::string> received;

    for (char byte : framed) {
        const auto part = decoder.feed(std::string(1, byte));
        received.insert(received.end(), part.begin(), part.end());
    }

    REQUIRE(received.size() == 1);
    CHECK(received.front() == payload);
}

TEST_CASE("frame decoder handles multiple messages in one read") {
    const auto first = serialize(Message{Hello{}});
    const auto second = serialize(Message{Move{Direction::West}});

    FrameDecoder decoder;
    const auto received = decoder.feed(
        frame_message(first) + frame_message(second)
    );

    REQUIRE(received.size() == 2);
    CHECK(received[0] == first);
    CHECK(received[1] == second);
}

TEST_CASE("invalid frame lengths are rejected") {
    FrameDecoder decoder;

    CHECK_THROWS_AS(
        decoder.feed(std::string(4, '\0')),
        std::invalid_argument
    );

    FrameDecoder oversized;
    const std::string invalid_header{
        '\0', '\0', '\x10', '\x01'
    };  // 4097 bytes

    CHECK_THROWS_AS(
        oversized.feed(invalid_header),
        std::invalid_argument
    );

    CHECK_THROWS_AS(
        frame_message(std::string(max_payload_size + 1, 'x')),
        std::length_error
    );
}

TEST_CASE("framing preserves the maximum allowed payload") {
    const std::string payload(max_payload_size, 'x');
    const auto framed = frame_message(payload);

    CHECK(framed.size() == max_payload_size + 4);

    FrameDecoder decoder;
    const auto messages = decoder.feed(framed);
    REQUIRE(messages.size() == 1);
    CHECK(messages[0] == payload);
}
