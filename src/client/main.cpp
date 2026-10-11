
#include <algorithm>
#include <string>

#include <raylib.h>

#include "retro/version.hpp"
#include "retro/world/position.hpp"
#include "retro/world/tile_map.hpp"

#include "sample_map.hpp"

namespace {

constexpr int tile_size = 16;
constexpr int virtual_width = 320;
constexpr int virtual_height = 192;

void draw_world(const retro::world::TileMap& map) {
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const retro::world::Position position{x, y};

            const bool walkable = map.is_walkable(position);

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

void draw_player(retro::world::Position position) {
    const int x = position.x * tile_size;
    const int y = position.y * tile_size;

    DrawRectangle(x + 3, y + 2, 10, 12, DARKBLUE);
    DrawRectangle(x + 5, y + 4, 6, 5, BEIGE);
    DrawRectangle(x + 4, y + 10, 8, 4, BLUE);
}

}  // namespace

int main() {
    const auto map = retro::client::make_sample_map();
    const retro::world::Position player_position{2, 2};

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
        // Render the world at its native pixel resolution.
        BeginTextureMode(canvas);
        ClearBackground(BLACK);

        draw_world(map);
        draw_player(player_position);

        EndTextureMode();

        // Scale using whole numbers to preserve pixel sharpness.
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
