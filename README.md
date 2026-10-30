# port-common

The game-agnostic layer of the Pokémon DS ports for Android/Linux (the Ayn Thor
builds): save slots, autosave, and, one module at a time, the rest of what is now in
pokeplatinum's `src/port/` (Gear companion screens, settings pages, cheats, save
editor, randomizer core, save sync, trade box…).

It is a meson subproject of each game:

| Game | Repo | Adapter |
|---|---|---|
| Pokémon Platinum | [SilasMayes/pokeplatinum](https://github.com/SilasMayes/pokeplatinum) `thor-android` | `src/port/game_platinum.c` |
| Pokémon HeartGold / SoulSilver | [SilasMayes/pokeheartgold](https://github.com/SilasMayes/pokeheartgold) `thor-android` | `src/port/game_hgss.c` (when HGSS needs a module) |

Both games build against the same libntr fork (`SilasMayes/libntr`, `thor-android`).

## How a game uses it

1. `subprojects/port-common.wrap` in the game (pinned to a commit, so each game commit
   builds with the port-common it was tested with):

   ```ini
   [wrap-git]
   url = https://github.com/SilasMayes/port-common.git
   revision = <commit>
   depth = 1
   directory = port-common

   [provide]
   dependency_names = port_common
   ```

2. In the game's `meson.build`, after libntr (port-common takes libntr's headers from the
   `NitroSDK` dependency, and the game's `build_target` option through `yield`):

   ```meson
   port_common_dep = subproject('port-common').get_variable('port_common_dep')
   ```

   and `port_common_dep` in the main executable's dependencies.

3. The game implements `include/port/port_game.h` once, in its adapter file.

## Rules

- **No game headers here.** port-common compiles with only its own `include/` and
  libntr's headers, so including a game header fails to build. Anything game-specific is
  a `PortGame_*` function in `port_game.h`.
- **The adapter grows with the modules.** A module moves here together with the
  `PortGame_*` functions it needs. Functions marked *transitional* call port modules
  still in the game and become direct calls once those modules move too.
- **Every game stays green.** A change here lands in a game by bumping that game's wrap
  revision, in a commit whose CI is green.
- Same file names and APIs as in the game where possible (`port/save_slots.h` keeps its
  include path), so a move doesn't touch every caller.

## Modules

| Module | Files | Moved from pokeplatinum |
|---|---|---|
| Save slots | `include/port/save_slots.h`, `src/save_slots.c` | 2026-10-30 |
| Autosave | `include/port/autosave.h`, `src/autosave.c` | 2026-10-30 |
| Save versions (Renegade Platinum Complete/Classic) | `include/port/save_version.h`, `src/save_version.c` | 2026-10-30 |
