// Autosave. See include/port/autosave.h
#include "port/autosave.h"

#include <nitro.h>
#include <SDL2/SDL.h>
#include <simulator/sim_lifecycle.h>
#include <stdio.h>

#include "port/port_game.h"

#define NOTICE_SHOW_MS 1800
#define NOTICE_FADE_MS 400

static u64 sLastSaveMs = 0;
static BOOL sSaving = FALSE;
static char sNotice[128];
static u64 sNoticeStartMs = 0;
static u64 sNoticeShowMs = NOTICE_SHOW_MS;

void Autosave_NoteSaved(void)
{
    sLastSaveMs = SDL_GetTicks64();
}

BOOL Autosave_CanSaveNow(const char **reason)
{
    const char *why = NULL;

    if (!PortGame_Field_IsRunning()) {
        why = "No game is running";
    } else if (sSaving) {
        why = "Already saving";
    } else if (!PortGame_Field_CanSave(&why)) {
        if (why == NULL) {
            why = "Saving isn't allowed here";
        }
    } else {
        why = NULL;
    }

    if (reason != NULL) {
        *reason = why;
    }
    return why == NULL;
}

BOOL Autosave_SaveNow(const char *notice)
{
    if (!Autosave_CanSaveNow(NULL)) {
        return FALSE;
    }

    sSaving = TRUE;
    BOOL saved = PortGame_Field_Save();
    sSaving = FALSE;

    if (saved) {
        printf("Autosave: saved (%s)\n", notice != NULL ? notice : "silent");
        if (notice != NULL) {
            Autosave_ShowNotice(notice);
        }
    } else {
        printf("Autosave: save failed\n");
        Autosave_ShowNotice("Save failed");
    }
    return saved;
}

void Autosave_FieldTick(void)
{
    PortAutosaveConfig config;
    if (!PortGame_Config_GetAutosave(&config) || !config.enabled) {
        return;
    }

    int minutes = config.minutes;
    if (minutes < 1) {
        minutes = 1;
    }

    u64 now = SDL_GetTicks64();
    if (now - sLastSaveMs < (u64)minutes * 60 * 1000) {
        return;
    }

    const char *reason = NULL;
    if (!Autosave_CanSaveNow(&reason)) {
        // Not safe this frame; try again on the next one
        static u64 sLastReasonLogMs = 0;
        if (now - sLastReasonLogMs > 10000) {
            sLastReasonLogMs = now;
            printf("Autosave: due, waiting (%s)\n", reason);
        }
        return;
    }

    Autosave_SaveNow("Autosaved");
}

// libntr calls this between frames when the app is about to pause or exit
void SIM_Prj_OnLifecycleEvent(int event)
{
    PortAutosaveConfig config;
    if (!PortGame_Config_GetAutosave(&config) || !config.onExit) {
        return;
    }

    const char *reason = NULL;
    if (!Autosave_CanSaveNow(&reason)) {
        if (PortGame_Field_IsRunning()) {
            printf("Autosave: not saving on %s (%s)\n",
                event == SIM_LIFECYCLE_QUIT ? "exit" : "background", reason);
        }
        return;
    }

    // Nobody sees a notice while the app goes away
    Autosave_SaveNow(NULL);
}

void Autosave_ShowNotice(const char *text)
{
    Autosave_ShowNoticeFor(text, NOTICE_SHOW_MS);
}

void Autosave_ShowNoticeFor(const char *text, int ms)
{
    snprintf(sNotice, sizeof(sNotice), "%s", text);
    sNoticeStartMs = SDL_GetTicks64();
    sNoticeShowMs = ms;
}

const char *Autosave_GetNotice(float *alpha)
{
    if (sNotice[0] == '\0') {
        return NULL;
    }

    u64 elapsed = SDL_GetTicks64() - sNoticeStartMs;
    if (elapsed >= sNoticeShowMs + NOTICE_FADE_MS) {
        sNotice[0] = '\0';
        return NULL;
    }

    *alpha = elapsed <= sNoticeShowMs ? 1.0f : 1.0f - (float)(elapsed - sNoticeShowMs) / NOTICE_FADE_MS;
    return sNotice;
}
