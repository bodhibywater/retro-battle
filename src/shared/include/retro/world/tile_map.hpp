#pragma once

#include <string>
#include <vector>

#include "retro/world/position.hpp"

namespace retro::world {

class TileMap {
public:
    explicit TileMap(std::vector<std::string> rows);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    [[nodiscard]] bool contains(Position position) const noexcept;
    [[nodiscard]] bool is_walkable(Position position) const noexcept;
    [[nodiscard]] char tile_at(Position position) const noexcept;

private:
    std::vector<std::string> rows_;
};

}  // namespace retro::world
