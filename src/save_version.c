// Versions of the game. See include/port/save_version.h
#include "port/save_version.h"

#include <nitro.h>
#include <simulator/sim_carts.h>
#include <stdio.h>
#include <string.h>

#include "port/port_game.h"
#include "port/save_slots.h"

static char sNames[SIM_CART_VERSIONS_MAX][SIM_CART_VERSION_NAME_LEN];
static int sInstalled = 0;
static u32 sGeneration = 1;
static BOOL sAsking = FALSE;

static int Refresh(void)
{
    int count = SIM_Carts_Versions(sNames, &sInstalled);
    return count > 1 ? count : 0;
}

int SaveVersion_Count(void)
{
    return Refresh();
}

const char *SaveVersion_Name(int index)
{
    int count = Refresh();
    return index >= 0 && index < count ? sNames[index] : "";
}

const char *SaveVersion_Description(int index)
{
    const char *description = PortGame_VersionDescription(SaveVersion_Name(index));
    return description != NULL ? description : "";
}

static int IndexOf(const char *name)
{
    int count = Refresh();
    for (int i = 0; i < count; i++) {
        if (strcmp(sNames[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

int SaveVersion_InUse(void)
{
    int index = IndexOf(SIM_Carts_VersionInUse());
    return index >= 0 ? index : sInstalled;
}

static void SidecarPath(int slot, char *out, int outSize)
{
    snprintf(out, outSize, "save_slot%d.version", slot + 1);
}

int SaveVersion_ForSlot(int slot)
{
    if (Refresh() == 0) {
        return 0;
    }
    char path[64];
    SidecarPath(slot, path, sizeof(path));
    FILE *file = fopen(path, "r");
    char line[64] = "";
    if (file != NULL) {
        if (fgets(line, sizeof(line), file) == NULL) {
            line[0] = '\0';
        }
        fclose(file);
    }
    line[strcspn(line, "\r\n")] = '\0';
    int index = IndexOf(line);
    return index >= 0 ? index : sInstalled;
}

static void Use(int index)
{
    if (Refresh() == 0) {
        return;
    }
    if (index != SaveVersion_InUse()) {
        sGeneration++;
    }
    SIM_Carts_UseVersion(sNames[index]);
}

void SaveVersion_OnSlotLoaded(int slot)
{
    if (Refresh() > 0) {
        Use(SaveVersion_ForSlot(slot));
    }
}

static void WriteSidecar(int slot, int index)
{
    char path[64];
    SidecarPath(slot, path, sizeof(path));
    FILE *file = fopen(path, "w");
    if (file != NULL) {
        fprintf(file, "%s\n", sNames[index]);
        fclose(file);
    }
}

void SaveVersion_OnSaved(int slot)
{
    if (Refresh() > 0) {
        WriteSidecar(slot, SaveVersion_InUse());
    }
}

void SaveVersion_ForgetSlot(int slot)
{
    char path[64];
    SidecarPath(slot, path, sizeof(path));
    remove(path);
}

void SaveVersion_SetForSlot(int slot, int index)
{
    if (Refresh() == 0 || index < 0 || index >= Refresh()) {
        return;
    }
    WriteSidecar(slot, index);
    if (slot == SaveSlots_Current()) {
        Use(index);
    }
}

u32 SaveVersion_Generation(void)
{
    return sGeneration;
}

void SaveVersion_AskForNewGame(BOOL canCancel)
{
    int count = Refresh();
    const char *names[SIM_CART_VERSIONS_MAX];
    const char *descriptions[SIM_CART_VERSIONS_MAX];
    for (int i = 0; i < count; i++) {
        names[i] = sNames[i];
        descriptions[i] = SaveVersion_Description(i);
    }
    SIM_Carts_AskVersion("Which version?",
        "This new game keeps the version you choose. (A save's version is in Settings > Saves.)", names,
        descriptions, count, canCancel);
    sAsking = TRUE;
}

int SaveVersion_TakeNewGameAnswer(void)
{
    if (!sAsking) {
        return -1;
    }
    int answer = SIM_Carts_TakeVersionAnswer();
    if (answer == -2) {
        return -2;
    }
    sAsking = FALSE;
    if (answer >= 0) {
        Use(answer);
    }
    return answer;
}
