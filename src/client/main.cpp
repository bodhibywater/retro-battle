
#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

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
    std::vector<int> held_keys;
    std::optional<Direction> queued_direction;
};

std::optional<Direction> direction_for_key(int key) {
    switch (key) {
        case KEY_W:
        case KEY_UP: return Direction::North;
        case KEY_S:
        case KEY_DOWN: return Direction::South;
        case KEY_A:
        case KEY_LEFT: return Direction::West;
        case KEY_D:
        case KEY_RIGHT: return Direction::East;
        default: return std::nullopt;
    }
}

void update_key_priority(PlayerState& player) {
    std::erase_if(player.held_keys, [](int key) {
        return !IsKeyDown(key);
    });

    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
        if (const auto direction = direction_for_key(key)) {
            std::erase(player.held_keys, key);
            if (IsKeyDown(key)) {
                player.held_keys.push_back(key);
            }
            player.queued_direction = direction;
        }
    }
}

void update_player(PlayerState& player, const TileMap& map) {
    update_key_priority(player);

    if (player.progress < 1.0f) {
        player.progress = std::min(
            1.0f,
            player.progress + GetFrameTime() / step_duration
        );
        if (player.progress < 1.0f) {
            return;
        }
    }

    auto direction = player.queued_direction;
    player.queued_direction.reset();
    if (!direction && !player.held_keys.empty()) {
        direction = direction_for_key(player.held_keys.back());
    }
    if (!direction) {
        return;
    }

    const Position next = try_move(map, player.tile, *direction);
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


Vector2 player_pixel_position(const PlayerState& player) {
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

    return Vector2{
        std::round(x),
        std::round(y)
    };
}

void draw_player(const PlayerState& player) {
    const Vector2 position = player_pixel_position(player);

    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);

    DrawRectangle(x + 3, y + 2, 10, 12, DARKBLUE);
    DrawRectangle(x + 5, y + 4, 6, 5, BEIGE);
    DrawRectangle(x + 4, y + 10, 8, 4, BLUE);
}

Camera2D make_camera(
    const TileMap& map,
    const PlayerState& player
) {
    const Vector2 player_position = player_pixel_position(player);

    const float world_width =
        static_cast<float>(map.width() * tile_size);

    const float world_height =
        static_cast<float>(map.height() * tile_size);

    const float half_width =
        static_cast<float>(virtual_width) / 2.0f;

    const float half_height =
        static_cast<float>(virtual_height) / 2.0f;

    // Centre the camera on the player's visual position.
    const float player_centre_x =
        player_position.x + static_cast<float>(tile_size) / 2.0f;

    const float player_centre_y =
        player_position.y + static_cast<float>(tile_size) / 2.0f;

    // Prevent the camera from showing space outside the map.
    const float camera_x = world_width > virtual_width
        ? std::clamp(
            player_centre_x,
            half_width,
            world_width - half_width
        )
        : world_width / 2.0f;

    const float camera_y = world_height > virtual_height
        ? std::clamp(
            player_centre_y,
            half_height,
            world_height - half_height
        )
        : world_height / 2.0f;

    Camera2D camera{};
    camera.target = Vector2{camera_x, camera_y};
    camera.offset = Vector2{half_width, half_height};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    return camera;
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

        const Camera2D camera = make_camera(map, player);

        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        BeginMode2D(camera);

        draw_world(map);
        draw_player(player);

        EndMode2D();

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
