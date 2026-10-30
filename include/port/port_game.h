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
// Save slot events for port modules still in the game (transitional)
// ---------------------------------------------------------------------------

// Right before a slot's save is (re)loaded (save sync fetches a newer copy here)
void PortGame_OnSlotBeforeLoad(int slot);

// After a slot's save has been loaded
void PortGame_OnSlotLoaded(int slot);

// After every successful save into a slot (manual or automatic)
void PortGame_OnSlotSaved(int slot);

// A slot's save was replaced from outside (import, save sync)
void PortGame_OnSlotReplaced(int slot);

#ifdef __cplusplus
}
#endif

#endif // PORT_COMMON_PORT_GAME_H
