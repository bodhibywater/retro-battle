
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

#include <raylib.h>

#include "retro/version.hpp"
#include "retro/world/direction.hpp"
#include "retro/world/movement.hpp"
#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

#include "sample_map.hpp"

namespace {

using retro::world::Direction;
using retro::world::Position;
using retro::world::TileMap;
using retro::world::try_move;

constexpr int tile_size = 16;
constexpr int virtual_width = 320;
constexpr int virtual_height = 192;

// Time taken to move one tile.
constexpr float step_duration = 0.16f;

struct PlayerState {
    Position tile{2, 2};
    Position previous_tile{2, 2};
    float progress{1.0f};
    std::optional<Direction> queued_direction;
};

std::optional<Direction> read_direction(bool pressed_only) {
    const auto active = [pressed_only](int arrow, int letter) {
        if (pressed_only) {
            return IsKeyPressed(arrow) || IsKeyPressed(letter);
        }

        return IsKeyDown(arrow) || IsKeyDown(letter);
    };

    if (active(KEY_UP, KEY_W)) {
        return Direction::North;
    }
    if (active(KEY_DOWN, KEY_S)) {
        return Direction::South;
    }
    if (active(KEY_LEFT, KEY_A)) {
        return Direction::West;
    }
    if (active(KEY_RIGHT, KEY_D)) {
        return Direction::East;
    }

    return std::nullopt;
}

void update_player(PlayerState& player, const TileMap& map) {
    // Remember a new direction pressed during an animation.
    if (const auto pressed = read_direction(true)) {
        player.queued_direction = pressed;
    }

    // Finish the current movement before starting another.
    if (player.progress < 1.0f) {
        player.progress = std::min(
            1.0f,
            player.progress + GetFrameTime() / step_duration
        );

        if (player.progress < 1.0f) {
            return;
        }
    }

    // Prefer a recent key press, then any held direction.
    auto direction = player.queued_direction;
    player.queued_direction.reset();

    if (!direction) {
        direction = read_direction(false);
    }

    if (!direction) {
        return;
    }

    const Position next = try_move(map, player.tile, *direction);

    // Collision or map boundary: stay where we are.
    if (next == player.tile) {
        return;
    }

    player.previous_tile = player.tile;
    player.tile = next;
    player.progress = 0.0f;
}

void draw_world(const TileMap& map) {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const bool walkable = map.is_walkable({x, y});

            const Color colour = walkable
                ? ((x + y) % 2 == 0
                    ? Color{153, 203, 129, 255}
                    : Color{164, 211, 140, 255})
                : Color{68, 99, 75, 255};

            DrawRectangle(
                x * tile_size,
                y * tile_size,
                tile_size,
                tile_size,
                colour
            );
        }
    }
}

void draw_player(const PlayerState& player) {
    // Interpolate between adjacent logical tile positions.
    const float x = std::lerp(
        static_cast<float>(player.previous_tile.x),
        static_cast<float>(player.tile.x),
        player.progress
    ) * static_cast<float>(tile_size);

    const float y = std::lerp(
        static_cast<float>(player.previous_tile.y),
        static_cast<float>(player.tile.y),
        player.progress
    ) * static_cast<float>(tile_size);

    // Snap to whole pixels for crisp pixel-art movement.
    const int draw_x = static_cast<int>(std::round(x));
    const int draw_y = static_cast<int>(std::round(y));

    DrawRectangle(draw_x + 3, draw_y + 2, 10, 12, DARKBLUE);
    DrawRectangle(draw_x + 5, draw_y + 4, 6, 5, BEIGE);
    DrawRectangle(draw_x + 4, draw_y + 10, 8, 4, BLUE);
}

}  // namespace

int main() {
    const auto map = retro::client::make_sample_map();
    PlayerState player;

    const std::string title =
        "Retro Battle v" + std::string{retro::version()};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 576, title.c_str());
    SetWindowMinSize(virtual_width, virtual_height);
    SetTargetFPS(60);

    RenderTexture2D canvas = LoadRenderTexture(
        virtual_width,
        virtual_height
    );

    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        update_player(player, map);

        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        draw_world(map);
        draw_player(player);

        EndTextureMode();

        const int scale = std::max(
            1,
            std::min(
                GetScreenWidth() / virtual_width,
                GetScreenHeight() / virtual_height
            )
        );

        const float draw_width =
            static_cast<float>(virtual_width * scale);

        const float draw_height =
            static_cast<float>(virtual_height * scale);

        const float offset_x =
            (static_cast<float>(GetScreenWidth()) - draw_width) / 2.0f;

        const float offset_y =
            (static_cast<float>(GetScreenHeight()) - draw_height) / 2.0f;

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTexturePro(
            canvas.texture,
            Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(virtual_width),
                -static_cast<float>(virtual_height)
            },
            Rectangle{
                offset_x,
                offset_y,
                draw_width,
                draw_height
            },
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();

    return 0;
}
