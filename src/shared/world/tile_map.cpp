
#include "retro/world/tile_map.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace retro::world {

TileMap::TileMap(std::vector<std::string> rows)
    : rows_(std::move(rows)) {

    if (rows_.empty() || rows_[0].empty()) {
        throw std::invalid_argument("Map must not be empty");
    }

    const auto width = rows_[0].size();

    if (width > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        rows_.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Map dimensions are too large");
    }

    for (const auto& row : rows_) {
        if (row.size() != width) {
            throw std::invalid_argument("Map rows must have equal width");
        }

        if (row.find_first_not_of(".#") != std::string::npos) {
            throw std::invalid_argument("Map contains an unknown tile");
        }
    }
}

int TileMap::width() const noexcept {
    return static_cast<int>(rows_[0].size());
}

int TileMap::height() const noexcept {
    return static_cast<int>(rows_.size());
}

bool TileMap::contains(Position position) const noexcept {
    return position.x >= 0 &&
           position.y >= 0 &&
           position.x < width() &&
           position.y < height();
}

bool TileMap::is_walkable(Position position) const noexcept {
    if (!contains(position)) {
        return false;
    }

    return rows_[static_cast<std::size_t>(position.y)]
                [static_cast<std::size_t>(position.x)] == '.';
}

}  // namespace retro::world
