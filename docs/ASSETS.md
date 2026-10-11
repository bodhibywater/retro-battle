# Overworld artwork

The five pixel-art PNG files under `assets/public/overworld/` were drawn from scratch for the initial Retro Battle demo. They are original project assets, not Pokémon assets or extracts from commercial games. No third-party asset pack is used in this tranche.

- `terrain.png`: seven 16×16 ground tiles in one horizontal row.
- `tree.png`: 16×32 tree; the trunk occupies one collision tile.
- `rock.png`: 16×16 rock.
- `house.png`: 80×64 building across a 5×4 collision footprint.
- `player.png`: 3 columns × 4 rows of 16×24 player frames (down, left, right, up).

The map is currently hand-constructed in `src/client/sample_map.hpp`, and tile collision is defined by `TileMap` in `src/shared`. Future maps may be created in Tiled without changing the movement rules.

The intended art direction is *inspired by* the cheerful pixel-art palette and top-down world feel of early handheld RPGs. The project does not redistribute Nintendo/Game Freak assets.

If we later replace or augment the graphics with CC0 art, **Kenney Tiny Town** (https://kenney.nl/assets/tiny-town) provides a compatible 16×16 starting point; record the actual source and licence for anything we incorporate.
