#include "retro/net/framing.hpp"

#include <stdexcept>

#include "retro/net/protocol.hpp"

namespace retro::net {

std::string frame_message(std::string_view payload) {
    if (payload.empty() || payload.size() > max_payload_size) {
        throw std::length_error("Invalid message length");
    }

    const auto length = static_cast<std::uint32_t>(payload.size());

    std::string frame;
    frame.reserve(4 + payload.size());

    frame.push_back(static_cast<char>((length >> 24) & 0xff));
    frame.push_back(static_cast<char>((length >> 16) & 0xff));
    frame.push_back(static_cast<char>((length >> 8) & 0xff));
    frame.push_back(static_cast<char>(length & 0xff));
    frame.append(payload);

    return frame;
}

std::vector<std::string> FrameDecoder::feed(std::string_view bytes) {
    std::vector<std::string> messages;

    for (char byte : bytes) {
        pending_.push_back(byte);

        if (expected_length_ == 0 && pending_.size() == 4) {
            std::uint32_t length = 0;

            for (char header_byte : pending_) {
                length = (length << 8) |
                    static_cast<unsigned char>(header_byte);
            }

            if (length == 0 || length > max_payload_size) {
                pending_.clear();
                throw std::invalid_argument("Invalid frame length");
            }

            expected_length_ = length;
        }

        if (expected_length_ != 0 &&
            pending_.size() == 4 + expected_length_) {
            messages.emplace_back(pending_.substr(4));
            pending_.clear();
            expected_length_ = 0;
        }
    }

    return messages;
}

}  // namespace retro::net
