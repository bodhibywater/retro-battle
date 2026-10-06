#include <string>

#include <raylib.h>

#include "retro/version.hpp"

int main() {
    constexpr int screen_width{960};
    constexpr int screen_height{540};

    const std::string window_title{
        "Retro Battle v" + std::string{retro::version()}
    };

    InitWindow(screen_width, screen_height, window_title.c_str());
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText(
            "RETRO BATTLE",
            40,
            40,
            40,
            BLACK
        );

        DrawText(
            "Client bootstrap",
            40,
            100,
            20,
            DARKGRAY
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}