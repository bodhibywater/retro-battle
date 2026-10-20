#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <raylib.h>

#include "retro/version.hpp"
#include "retro/world/direction.hpp"
#include "retro/world/player_state.hpp"
#include "retro/world/position.hpp"
#include "retro/world/sample_map.hpp"
#include "retro/world/tile_map.hpp"

namespace {

using retro::world::Direction;
using retro::world::Position;
using retro::world::TileMap;

constexpr int tile_size = 16;
constexpr int virtual_width = 320;
constexpr int virtual_height = 192;
constexpr float step_duration = 0.16f;

struct PlayerState {
    // Authoritative game state is platform-independent.
    retro::world::PlayerState world{retro::world::sample_spawn, Direction::South};

    // Client-only animation and keyboard state.
    Position previous_tile{retro::world::sample_spawn};
    float progress{1.0f};
    std::vector<int> held_keys;
    std::optional<Direction> queued_direction;
};

struct Assets {
    Texture2D terrain{};
    Texture2D tree{};
    Texture2D rock{};
    Texture2D house{};
    Texture2D player{};
};

std::optional<Direction> direction_for_key(int key) {
    switch (key) {
        case KEY_W:
        case KEY_UP:
            return Direction::North;
        case KEY_S:
        case KEY_DOWN:
            return Direction::South;
        case KEY_A:
        case KEY_LEFT:
            return Direction::West;
        case KEY_D:
        case KEY_RIGHT:
            return Direction::East;
        default:
            return std::nullopt;
    }
}

// Keep keys in press order. The most recently pressed *held* key wins;
// releasing it falls back to the previously held key.
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

    const Position previous = player.world.position;
    if (!retro::world::move_player(player.world, map, *direction)) {
        return;
    }

    player.previous_tile = previous;
    player.progress = 0.0f;
}

Vector2 player_pixel_position(const PlayerState& player) {
    return Vector2{
        std::round(std::lerp(
            static_cast<float>(player.previous_tile.x),
            static_cast<float>(player.world.position.x), player.progress
        ) * static_cast<float>(tile_size)),
        std::round(std::lerp(
            static_cast<float>(player.previous_tile.y),
            static_cast<float>(player.world.position.y), player.progress
        ) * static_cast<float>(tile_size))
    };
}

Camera2D make_camera(const TileMap& map, const PlayerState& player) {
    const Vector2 pos = player_pixel_position(player);
    const float world_width = static_cast<float>(map.width() * tile_size);
    const float world_height = static_cast<float>(map.height() * tile_size);
    constexpr float half_width = static_cast<float>(virtual_width) / 2.0f;
    constexpr float half_height = static_cast<float>(virtual_height) / 2.0f;

    Camera2D camera{};
    camera.target = Vector2{
        world_width > virtual_width
            ? std::clamp(pos.x + 8.0f, half_width, world_width - half_width)
            : world_width / 2.0f,
        world_height > virtual_height
            ? std::clamp(pos.y + 8.0f, half_height, world_height - half_height)
            : world_height / 2.0f
    };
    camera.offset = Vector2{half_width, half_height};
    camera.zoom = 1.0f;
    return camera;
}

Texture2D load_asset(const std::filesystem::path& dir, const char* file) {
    const std::string path = (dir / file).string();
    return LoadTexture(path.c_str());
}

Assets load_assets() {
    const auto asset_dir = std::filesystem::path(GetApplicationDirectory())
        / "assets" / "overworld";
    return Assets{
        load_asset(asset_dir, "terrain.png"),
        load_asset(asset_dir, "tree.png"),
        load_asset(asset_dir, "rock.png"),
        load_asset(asset_dir, "house.png"),
        load_asset(asset_dir, "player.png")
    };
}

bool assets_loaded(const Assets& assets) {
    return assets.terrain.id && assets.tree.id && assets.rock.id &&
           assets.house.id && assets.player.id;
}

void unload_assets(const Assets& assets) {
    for (const Texture2D tex : {
             assets.terrain, assets.tree, assets.rock, assets.house, assets.player
         }) {
        if (tex.id) {
            UnloadTexture(tex);
        }
    }
}

void draw_ground(const TileMap& map, const Assets& assets) {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const char tile = map.tile_at({x, y});
            int index = ((x * 13 + y * 29) % 7 == 0) ? 1 : 0;
            if (tile == 'p' || tile == '.') {
                index = 2;
            } else if (tile == 'f') {
                index = 3;
            } else if (tile == 'w') {
                index = 4;
            }
            DrawTextureRec(
                assets.terrain,
                Rectangle{static_cast<float>(index * tile_size), 0.0f,
                          static_cast<float>(tile_size), static_cast<float>(tile_size)},
                Vector2{static_cast<float>(x * tile_size),
                        static_cast<float>(y * tile_size)},
                WHITE
            );
        }
    }
    // Buildings use a 5×4-tile sprite; their blocked footprint is in TileMap.
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.tile_at({x, y}) == 'h') {
                DrawTexture(assets.house, x * tile_size, y * tile_size, WHITE);
            }
        }
    }
}

void draw_player(const PlayerState& player, const Assets& assets) {
    const Vector2 pos = player_pixel_position(player);
    int row = 0;
    switch (player.world.facing) {
        case Direction::South: row = 0; break;
        case Direction::West: row = 1; break;
        case Direction::East: row = 2; break;
        case Direction::North: row = 3; break;
    }
    const int frame = player.progress < 1.0f
        ? (player.progress < 0.5f ? 1 : 2) : 0;
    DrawTextureRec(
        assets.player,
        Rectangle{static_cast<float>(frame * 16), static_cast<float>(row * 24),
                  16.0f, 24.0f},
        Vector2{pos.x, pos.y - 8.0f},
        WHITE
    );
}

// Tall trees overlap the row above them. Draw obstacles in row order,
// inserting the player at the corresponding depth for a natural overlap.
void draw_objects_and_player(const TileMap& map, const PlayerState& player,
                             const Assets& assets) {
    const float player_feet = player_pixel_position(player).y + tile_size;
    bool player_drawn = false;
    for (int y = 0; y < map.height(); ++y) {
        if (!player_drawn && player_feet <= static_cast<float>(y * tile_size)) {
            draw_player(player, assets);
            player_drawn = true;
        }
        for (int x = 0; x < map.width(); ++x) {
            const char tile = map.tile_at({x, y});
            if (tile == 't') {
                DrawTexture(assets.tree, x * tile_size, y * tile_size - tile_size, WHITE);
            } else if (tile == 'r') {
                DrawTexture(assets.rock, x * tile_size, y * tile_size, WHITE);
            }
        }
    }
    if (!player_drawn) {
        draw_player(player, assets);
    }
}

}  // namespace

int main() {
    const auto map = retro::world::make_sample_map();
    PlayerState player;
    const std::string title = "Retro Battle v" + std::string{retro::version()};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(960, 576, title.c_str());

    if (!IsWindowReady()) {
        std::cerr << "Failed to initialise the graphics window. "
                << "Check that a graphical display is available.\n";
        return 1;
    }

    SetWindowMinSize(virtual_width, virtual_height);
    SetTargetFPS(60);

    const Assets assets = load_assets();
    if (!assets_loaded(assets)) {
        std::cerr << "Unable to load overworld assets next to retro_client.\n";
        unload_assets(assets);
        CloseWindow();
        return 1;
    }

    const RenderTexture2D canvas = LoadRenderTexture(virtual_width, virtual_height);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        update_player(player, map);
        const Camera2D camera = make_camera(map, player);

        BeginTextureMode(canvas);
        ClearBackground(BLACK);
        BeginMode2D(camera);
        draw_ground(map, assets);
        draw_objects_and_player(map, player, assets);
        EndMode2D();
        EndTextureMode();

        const int scale = std::max(1, std::min(
            GetScreenWidth() / virtual_width,
            GetScreenHeight() / virtual_height
        ));
        const float draw_width = static_cast<float>(virtual_width * scale);
        const float draw_height = static_cast<float>(virtual_height * scale);
        const float offset_x =
            (static_cast<float>(GetScreenWidth()) - draw_width) / 2.0f;
        const float offset_y =
            (static_cast<float>(GetScreenHeight()) - draw_height) / 2.0f;

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.texture,
            Rectangle{0.0f, 0.0f, static_cast<float>(virtual_width),
                      -static_cast<float>(virtual_height)},
            Rectangle{offset_x, offset_y, draw_width, draw_height},
            Vector2{0.0f, 0.0f}, 0.0f, WHITE
        );
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    unload_assets(assets);
    CloseWindow();
    return 0;
}
