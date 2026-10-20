#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace retro::net {

// Four-byte unsigned big-endian payload length followed by UTF-8 JSON.
// Throws std::length_error if the payload is empty or too large.
[[nodiscard]] std::string frame_message(std::string_view payload);

// Handles partial TCP reads and multiple frames in a single read.
// Throws std::invalid_argument on invalid frame lengths;
// the caller should close that connection.
class FrameDecoder {
public:
    [[nodiscard]] std::vector<std::string> feed(std::string_view bytes);

private:
    std::string pending_;
    std::uint32_t expected_length_{};
};

}  // namespace retro::net
