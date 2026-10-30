#ifndef PORT_COMMON_AUTOSAVE_H
#define PORT_COMMON_AUTOSAVE_H

// Autosave: runs the game's own save (the same one as SAVE in the start menu) when it is
// safe to, the way the gen1recomp quick save does:
//   - on the overworld, with the player in control: no script, menu, warp or other
//     field task running, no sub-application (battle, bag...), player standing still
//   - only where the start menu offers SAVE (not in the Safari Zone, Pal Park, Battle
//     Tower, Union Room, Colosseum or Underground) and with no wireless session
//   - only once the slot has been saved normally at least once, and never over a
//     different game (a new game started on top of an existing save)
// It runs every few minutes (setting) and when the app is closed or sent to the
// background (setting).
//
// The game-specific part ("is the player free on the overworld, may the game save
// here") is the game adapter's PortGame_Field_CanSave (port/port_game.h).

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Resets the autosave timer (called after every save or load)
void Autosave_NoteSaved(void);

// Called every overworld frame in which the player has control
void Autosave_FieldTick(void);

// Whether the game can be saved right now, and why not (short text) if it can't
BOOL Autosave_CanSaveNow(const char **reason);

// Saves now if it is safe; shows notice on screen when it did
BOOL Autosave_SaveNow(const char *notice);

// On-screen notice ("Autosaved"): text and opacity 0..1, or NULL when none is showing
const char *Autosave_GetNotice(float *alpha);
void Autosave_ShowNotice(const char *text);
void Autosave_ShowNoticeFor(const char *text, int ms);

#ifdef __cplusplus
}
#endif

#endif // PORT_COMMON_AUTOSAVE_H
