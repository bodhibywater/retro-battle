# Retro Battle

A small multiplayer C++ game inspired by early handheld RPGs.

The goal is to build a shared pixel-art world where players can meet and challenge each other to different multiplayer games. The first planned game mode is turn-based creature battles.

## Status

**Early development — project bootstrap.**

Currently implemented:

- C++20 client, server and shared-library targets
- Basic raylib window
- Boost.Asio server runtime initialization
- Catch2 unit testing
- Linux and Windows CI builds

The overworld, networking protocol and battle mechanics are not yet implemented.

## Technology

- **C++20** — client, server and game logic
- **raylib** — graphics, input and audio
- **Boost.Asio** — networking
- **CMake + Ninja** — building
- **vcpkg** — dependency management
- **Catch2 + CTest** — automated testing

JSON-based game data and SQLite persistence are planned for later development.

## Building

### Prerequisites

- CMake 3.25+
- C++20-compatible compiler (GCC or MSVC)
- Ninja
- Git
- vcpkg

On Linux, install the native development libraries required by raylib. The GitHub Actions workflow provides an Ubuntu reference.

Clone and bootstrap vcpkg separately:

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
git checkout 886afac8da9e742ec2cde32f6300135614025e1b
```

On Linux:

```bash
./bootstrap-vcpkg.sh -disableMetrics
```

On Windows:

```powershell
.\bootstrap-vcpkg.bat -disableMetrics
```

Set `VCPKG_ROOT` to the absolute path of that checkout.

### Debug build

From the project root:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

### Release build

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

### Run

On Linux:

```bash
./build/dev/retro_client
./build/dev/retro_server
```

On Windows, run `retro_client.exe` and `retro_server.exe` from `build/dev`.

The client currently displays a test window. The server initializes its networking runtime and exits; it does not yet accept connections.

## Development

VS Code is supported through the committed CMake presets and recommended extensions.

The development workflow is:

1. Create a feature branch from `main`.
2. Make small, logical commits.
3. Run the relevant local build and tests.
4. Open a pull request.
5. Ensure Linux and Windows CI pass.
6. Review and merge through GitHub.

Direct development on `main` is discouraged.

## Architecture

```text
src/
├── client/      # Rendering, input and presentation
├── server/      # Authoritative multiplayer server
└── shared/      # Common game logic and protocol

tests/           # Automated tests
```

Future game modes should reuse the challenge/session/result lifecycle without requiring identical game simulations.

## Assets and licensing

The public repository is intended to contain only original or appropriately licensed content.

Local content packs under `assets/local/` and `data/local/` are excluded from Git. Proprietary third-party game assets must not be committed or redistributed without appropriate permission.

## Roadmap

Initial milestones:

- Shared tile-based overworld
- Multiplayer connections and movement
- Turn-based creature battle engine
- Player challenges and battle sessions
- Persistent player profiles and leaderboard
- Additional game modes in later versions

The priority is a small, reliable multiplayer experience rather than a large feature set.
