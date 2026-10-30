// See include/port/nuzlocke.h
#include "port/nuzlocke.h"

#include <nitro.h>
#include <stdio.h>
#include <string.h>

#include "port/port_game.h"
#include "port/save_slots.h"

static NuzlockeRun sRun;
static int sLoadedSlot = -1;
static int sCurrent = NUZLOCKE_ENC_NONE; // the wild battle going on

// The settings; all off while they aren't loaded
static PortNuzlockeConfig Config(void)
{
    PortNuzlockeConfig config;
    if (!PortGame_Config_GetNuzlocke(&config)) {
        memset(&config, 0, sizeof(config));
    }
    return config;
}

BOOL Nuzlocke_Enabled(void)
{
    return Config().enabled;
}

static void FilePath(int slot, char *out, size_t size)
{
    snprintf(out, size, "save_slot%d.nuzlocke", slot + 1);
}

static void Save(void)
{
    char path[64];
    FilePath(sLoadedSlot, path, sizeof(path));
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return;
    }
    for (int i = 0; i < sRun.areaCount; i++) {
        const NuzlockeArea *area = &sRun.areas[i];
        fprintf(file, "area=%u,%u,%u\n", area->location, area->species, area->result);
    }
    for (int i = 0; i < sRun.deathCount; i++) {
        const NuzlockeDeath *death = &sRun.deaths[i];
        fprintf(file, "death=%u,%u,%u,%u,%s\n", (unsigned)death->personality, death->species, death->level, death->location,
            death->name);
    }
    fclose(file);
}

static void Load(int slot)
{
    memset(&sRun, 0, sizeof(sRun));
    sLoadedSlot = slot;
    char path[64];
    FilePath(slot, path, sizeof(path));
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), file) != NULL) {
        unsigned a, b, c, d;
        char name[32] = "";
        if (sscanf(line, "area=%u,%u,%u", &a, &b, &c) == 3 && sRun.areaCount < NUZLOCKE_MAX_AREAS) {
            NuzlockeArea *area = &sRun.areas[sRun.areaCount++];
            area->location = (u16)a;
            area->species = (u16)b;
            area->result = (u8)c;
        } else if (sscanf(line, "death=%u,%u,%u,%u,%15[^\n]", &a, &b, &c, &d, name) >= 4
            && sRun.deathCount < NUZLOCKE_MAX_DEATHS) {
            NuzlockeDeath *death = &sRun.deaths[sRun.deathCount++];
            death->personality = a;
            death->species = (u16)b;
            death->level = (u8)c;
            death->location = (u16)d;
            snprintf(death->name, sizeof(death->name), "%s", name);
        }
    }
    fclose(file);
}

// The slot's file was replaced (save sync): read it again when next needed
void Nuzlocke_ForgetLoaded(void)
{
    sLoadedSlot = -1;
}

static void EnsureLoaded(void)
{
    if (SaveSlots_Current() != sLoadedSlot) {
        Load(SaveSlots_Current());
    }
}

const NuzlockeRun *Nuzlocke_Get(void)
{
    EnsureLoaded();
    return &sRun;
}

static NuzlockeArea *FindArea(u16 location)
{
    for (int i = 0; i < sRun.areaCount; i++) {
        if (sRun.areas[i].location == location) {
            return &sRun.areas[i];
        }
    }
    return NULL;
}

const NuzlockeArea *Nuzlocke_Area(u16 location)
{
    EnsureLoaded();
    return FindArea(location);
}

int Nuzlocke_CurrentEncounter(void)
{
    return Nuzlocke_Enabled() ? sCurrent : NUZLOCKE_ENC_NONE;
}

BOOL Nuzlocke_IsDead(u32 personality)
{
    EnsureLoaded();
    for (int i = 0; i < sRun.deathCount; i++) {
        if (sRun.deaths[i].personality == personality) {
            return TRUE;
        }
    }
    return FALSE;
}

void Nuzlocke_Reset(void)
{
    EnsureLoaded();
    memset(&sRun, 0, sizeof(sRun));
    Save();
}

static u16 CurrentLocation(void)
{
    return PortGame_Field_LocationName();
}

void Nuzlocke_OnBattleStart(u32 battleType, struct Party *wildParty)
{
    sCurrent = NUZLOCKE_ENC_NONE;
    if (!Nuzlocke_Enabled() || !PortGame_Battle_IsWild(battleType) || wildParty == NULL
        || PortGame_Party_Count(wildParty) == 0) {
        return;
    }
    EnsureLoaded();
    const PortNuzlockeConfig config = Config();
    PortMonInfo info;
    PortGame_Mon_GetInfo(PortGame_Party_Mon(wildParty, 0), &info);
    const u16 species = info.species;
    const u16 location = CurrentLocation();

    if (config.shiny && info.shiny) {
        sCurrent = NUZLOCKE_ENC_SHINY;
        return;
    }
    if (FindArea(location) != NULL) {
        sCurrent = NUZLOCKE_ENC_USED;
        return;
    }
    if (config.dupes && PortGame_Pokedex_HasCaught(species)) {
        sCurrent = NUZLOCKE_ENC_DUPE;
        return;
    }
    if (sRun.areaCount < NUZLOCKE_MAX_AREAS) {
        NuzlockeArea *area = &sRun.areas[sRun.areaCount++];
        area->location = location;
        area->species = species;
        area->result = NUZLOCKE_IN_BATTLE;
        Save();
    }
    sCurrent = NUZLOCKE_ENC_FIRST;
}

void Nuzlocke_OnBattleEnd(u32 battleType, int resultMask, struct Party *playerParty, struct Party *wildParty)
{
    const int encounter = sCurrent;
    sCurrent = NUZLOCKE_ENC_NONE;
    if (!Nuzlocke_Enabled()) {
        return;
    }
    EnsureLoaded();
    BOOL changed = FALSE;

    // The area's first encounter: caught or missed
    if (encounter == NUZLOCKE_ENC_FIRST) {
        NuzlockeArea *area = FindArea(CurrentLocation());
        if (area != NULL && area->result == NUZLOCKE_IN_BATTLE) {
            area->result = PortGame_Battle_Captured(resultMask) ? NUZLOCKE_CAUGHT : NUZLOCKE_MISSED;
            changed = TRUE;
        }
    }

    // Who fainted
    if (playerParty != NULL) {
        const PortNuzlockeConfig config = Config();
        int alive = 0;
        for (int i = 0; i < PortGame_Party_Count(playerParty); i++) {
            PortMonInfo info;
            PortGame_Mon_GetInfo(PortGame_Party_Mon(playerParty, i), &info);
            if (!info.isEgg && info.hp > 0) {
                alive++;
            }
        }
        for (int i = PortGame_Party_Count(playerParty) - 1; i >= 0; i--) {
            struct Pokemon *mon = PortGame_Party_Mon(playerParty, i);
            PortMonInfo info;
            PortGame_Mon_GetInfo(mon, &info);
            if (info.isEgg || info.hp > 0) {
                continue;
            }
            if (!Nuzlocke_IsDead(info.personality) && sRun.deathCount < NUZLOCKE_MAX_DEATHS) {
                NuzlockeDeath *death = &sRun.deaths[sRun.deathCount++];
                memset(death, 0, sizeof(*death));
                death->personality = info.personality;
                death->species = info.species;
                death->level = info.level;
                death->location = CurrentLocation();
                PortGame_Mon_GetName(mon, death->name, sizeof(death->name));
                changed = TRUE;
            }
            // Off to the PC, while someone is left to carry on
            if (config.toPC && alive > 0 && !PortGame_Battle_Lost(resultMask)) {
                PortGame_Party_MoveToPC(playerParty, i);
            }
        }
    }

    (void)wildParty;
    (void)battleType;
    if (changed) {
        Save();
    }
}
