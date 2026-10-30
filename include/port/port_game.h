#ifndef PORT_COMMON_PORT_GAME_H
#define PORT_COMMON_PORT_GAME_H

// The game adapter: everything port-common needs from the game it is built into.
// port-common never includes a game header; each game implements this file once:
//   pokeplatinum   src/port/game_platinum.c
//   pokeheartgold  src/port/game_hgss.c
//
// It grows one module at a time, as modules move from a game's src/port/ into
// port-common. Functions marked "transitional" call port modules that are still in
// the game; they go away (into direct calls) when those modules move here too.

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// The game's save object (SaveData in both games). port-common only passes it back.
struct SaveData;

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------

// For messages: "Pokemon Platinum" / "Platinum"
const char *PortGame_Name(void);
const char *PortGame_ShortName(void);

// ---------------------------------------------------------------------------
// Settings the game keeps (sim_config_prj.ini on Platinum)
// ---------------------------------------------------------------------------

// Save slot in use, 1-based (slot 1 is the original save.bin)
int PortGame_Config_GetSaveSlot(void);

// Stores the slot in use (1-based) and writes the settings file
void PortGame_Config_SetSaveSlot(int slot);

typedef struct PortAutosaveConfig {
    BOOL enabled;    // save automatically while exploring, when it is safe to
    int minutes;     // how often
    BOOL onExit;     // also save when the app is closed or sent to the background
} PortAutosaveConfig;

// FALSE while the settings aren't loaded yet
BOOL PortGame_Config_GetAutosave(PortAutosaveConfig *out);

typedef struct PortCheatsConfig {
    int shinyOdds;      // 1 in this many; 0 or worse than the game's: the game's odds
    int expPercent;     // EXP gained, in percent (100 or 0: unchanged)
    BOOL alwaysCatch;
    BOOL infinitePP;
    BOOL alwaysEscape;
    BOOL fastEggs;
    BOOL anyTM;
    BOOL expShareAll;   // Exp. Share for the whole party (a Gameplay setting)
} PortCheatsConfig;

// FALSE while the settings aren't loaded yet (every cheat is off then)
BOOL PortGame_Config_GetCheats(PortCheatsConfig *out);

typedef struct PortNuzlockeConfig {
    BOOL enabled;   // the Nuzlocke helper (off by default)
    BOOL dupes;     // dupes clause
    BOOL shiny;     // shiny clause
    BOOL toPC;      // fainted Pokemon go to the PC after the battle
} PortNuzlockeConfig;

// FALSE while the settings aren't loaded yet (the helper is off then)
BOOL PortGame_Config_GetNuzlocke(PortNuzlockeConfig *out);

// ---------------------------------------------------------------------------
// Save data
// ---------------------------------------------------------------------------

// What the save slot screens show about a save
typedef struct PortSaveSummary {
    char name[16];   // player name, ASCII
    int hours;
    int minutes;
    int badges;
    BOOL pokedexObtained;
    int seen;        // Pokédex: species seen
} PortSaveSummary;

// FALSE when the loaded save holds no game (nothing to summarize)
BOOL PortGame_Save_GetSummary(struct SaveData *saveData, PortSaveSummary *out);

// Reloads the save data from the save file (the path set with SIM_Backup_SetPath)
void PortGame_Save_Reload(struct SaveData *saveData);

// Whether a raw flash image holds a valid save of this game
BOOL PortGame_Save_IsValidImage(const u8 *image, u32 size);

// ---------------------------------------------------------------------------
// Overworld
// ---------------------------------------------------------------------------

// TRUE while the overworld exists (a game is running; FALSE on the title screen and
// main menu)
BOOL PortGame_Field_IsRunning(void);

// The "player is free on the overworld" gate: TRUE when the game could save right now
// the way the start menu's SAVE does. Only called while the overworld is running. When
// it returns FALSE, *reason (if not NULL) gets a short reason for the settings screen,
// e.g. "Player is moving".
BOOL PortGame_Field_CanSave(const char **reason);

// Runs the game's own save (the start menu's SAVE). TRUE on success.
BOOL PortGame_Field_Save(void);

// ---------------------------------------------------------------------------
// Versions (save_version.c; libntr's carts)
// ---------------------------------------------------------------------------
// The game also implements libntr's hooks SIM_Prj_VersionFiles (the files a version may
// differ in) and SIM_Prj_VersionName (a version patch's name, e.g. "Classic").

// One or two sentences for the "Which version?" question, or NULL
const char *PortGame_VersionDescription(const char *versionName);

// ---------------------------------------------------------------------------
// Pokémon (cheats, shiny hunt, Nuzlocke)
// ---------------------------------------------------------------------------
// The game's own party and Pokémon objects; port-common only passes them back.

struct Party;
struct Pokemon;

// Highest species ID of the Gen 4 games: the 493 of the National Dex, then Egg (494) and
// Bad Egg (495), as both Platinum and HGSS number them (the game's MAX_SPECIES). Species IDs
// in the sidecar files are these numbers. The adapter checks it against the game's own.
#define PORT_SPECIES_MAX 495

int PortGame_Party_Count(struct Party *party);
struct Pokemon *PortGame_Party_Mon(struct Party *party, int index);

typedef struct PortMonInfo {
    u16 species;      // 0: none
    u8 level;
    BOOL isEgg;
    BOOL shiny;       // at the game's own threshold (a ROM hack may change it)
    u32 hp;
    u32 personality;
    u32 otID;
} PortMonInfo;

void PortGame_Mon_GetInfo(struct Pokemon *mon, PortMonInfo *out);

// Nickname in ASCII (Nidoran's symbols as " M"/" F")
void PortGame_Mon_GetName(struct Pokemon *mon, char *out, int outSize);

// Gives the Pokémon a new personality value and recomputes what depends on it
void PortGame_Mon_SetPersonality(struct Pokemon *mon, u32 personality);

// Whether a Pokémon with this personality is shiny for trainer otID (game's threshold)
BOOL PortGame_IsPersonalityShiny(u32 otID, u32 personality);

// The game's own shiny odds: 1 in this many (8192 in vanilla Gen 4; ROM hacks differ)
u32 PortGame_BaseShinyOdds(void);

// Moves a party Pokémon to the first free PC box slot (and asks for a full save next
// time). FALSE, party unchanged, when the PC is full or no save is loaded.
BOOL PortGame_Party_MoveToPC(struct Party *party, int index);

// The player's trainer ID (the full 32 bits Pokémon store as OT ID); FALSE with no save
BOOL PortGame_PlayerTrainerID(u32 *out);

// Whether the loaded save's Pokédex has the species caught (FALSE with no game running)
BOOL PortGame_Pokedex_HasCaught(u16 species);

// The overworld's current location name ID (map label), or 0xFFFF with no game running.
// The Nuzlocke books are kept per location name.
u16 PortGame_Field_LocationName(void);

// Battles: the game's battle type flags and result mask, as its hooks pass them
BOOL PortGame_Battle_IsWild(u32 battleType);   // not trainer, link, frontier, Pal Park, tutorial
BOOL PortGame_Battle_Captured(int resultMask); // the wild Pokémon was caught
BOOL PortGame_Battle_Lost(int resultMask);     // the player lost

#ifdef __cplusplus
}
#endif

#endif // PORT_COMMON_PORT_GAME_H
