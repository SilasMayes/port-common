// See include/port/cheats.h
#include "port/cheats.h"

#include <nitro.h>
#include <string.h>
#include <time.h>

#include "port/port_game.h"

// The settings; all off (zero) while they aren't loaded
static PortCheatsConfig Config(void)
{
    PortCheatsConfig config;
    if (!PortGame_Config_GetCheats(&config)) {
        memset(&config, 0, sizeof(config));
    }
    return config;
}

static u32 GameShinyOdds(void)
{
    return PortGame_BaseShinyOdds();
}

u32 Cheats_ShinyOdds(void)
{
    const u32 game = GameShinyOdds();
    const PortCheatsConfig config = Config();
    if (config.shinyOdds <= 0 || (u32)config.shinyOdds >= game) {
        return game;
    }
    return (u32)config.shinyOdds;
}

BOOL Cheats_ShinyOddsBoosted(void)
{
    return Cheats_ShinyOdds() < GameShinyOdds();
}

u32 Cheats_ScaleExp(u32 exp)
{
    const PortCheatsConfig config = Config();
    if (config.expPercent <= 0 || config.expPercent == 100) {
        return exp;
    }
    const u64 scaled = (u64)exp * (u32)config.expPercent / 100u;
    return scaled > 0xFFFFFFu ? 0xFFFFFFu : (u32)scaled;
}

BOOL Cheats_AlwaysCatch(void)
{
    return Config().alwaysCatch;
}

BOOL Cheats_InfinitePP(void)
{
    return Config().infinitePP;
}

BOOL Cheats_AlwaysEscape(void)
{
    return Config().alwaysEscape;
}

BOOL Cheats_FastEggs(void)
{
    return Config().fastEggs;
}

BOOL Cheats_AnyTM(void)
{
    return Config().anyTM;
}

BOOL Cheats_ExpShareAll(void)
{
    return Config().expShareAll;
}

// --- shiny odds ----------------------------------------------------------------------------

// Our own random numbers: the game's RNG stays as it would have been
static u32 sRandom = 0;

static u32 Random(void)
{
    if (sRandom == 0) {
        sRandom = (u32)time(NULL) * 2654435761u | 1u;
    }
    // xorshift32
    sRandom ^= sRandom << 13;
    sRandom ^= sRandom >> 17;
    sRandom ^= sRandom << 5;
    return sRandom;
}

// A shiny personality as close to the old one as it gets: the low byte (gender, ability)
// stays, and so does the nature whenever it can (it nearly always can)
static u32 ShinyPersonality(u32 otID, u32 personality)
{
    const u32 trainerValue = (otID >> 16) ^ (otID & 0xFFFF);
    const u32 nature = personality % 25;
    const u32 low = personality & 0xFF;
    const u32 middle = (personality >> 8) & 0xFF;

    for (u32 i = 0; i < 256; i++) {
        const u32 lo = low | (((middle + i) & 0xFF) << 8);
        for (u32 x = 0; x < 8; x++) {
            const u32 hi = (trainerValue ^ lo ^ x) & 0xFFFF;
            const u32 candidate = (hi << 16) | lo;
            if (candidate % 25 == nature) {
                return candidate;
            }
        }
    }
    const u32 lo = personality & 0xFFFF;
    return (((trainerValue ^ lo) & 0xFFFF) << 16) | lo;
}

// Shiny at the boosted odds for the trainer otID: what's left after the game's own roll
static void MaybeMakeShiny(struct Pokemon *mon, u32 otID)
{
    PortMonInfo info;
    PortGame_Mon_GetInfo(mon, &info);
    if (!Cheats_ShinyOddsBoosted() || PortGame_IsPersonalityShiny(otID, info.personality)) {
        return;
    }
    const u32 odds = Cheats_ShinyOdds();
    const u32 game = GameShinyOdds();
    BOOL shiny;
    if (odds <= 1) {
        shiny = TRUE;
    } else {
        // P(shiny) = 1/odds overall, the game already gave 1/game: (1/odds - 1/game) / (1 - 1/game)
        const double wanted = (1.0 / odds - 1.0 / game) / (1.0 - 1.0 / game);
        shiny = (double)(Random() >> 8) / (double)(1u << 24) < wanted;
    }
    if (!shiny) {
        return;
    }

    const u32 shinyPersonality = ShinyPersonality(otID, info.personality);
    if (PortGame_IsPersonalityShiny(otID, shinyPersonality)) {
        PortGame_Mon_SetPersonality(mon, shinyPersonality);
    }
}

void Cheats_OnBattleStart(u32 battleType, struct Party *wildParty)
{
    if (wildParty == NULL || !PortGame_Battle_IsWild(battleType)) {
        return;
    }
    for (int i = 0; i < PortGame_Party_Count(wildParty); i++) {
        struct Pokemon *mon = PortGame_Party_Mon(wildParty, i);
        PortMonInfo info;
        PortGame_Mon_GetInfo(mon, &info);
        if (info.species != 0 && info.species <= PORT_SPECIES_MAX) {
            MaybeMakeShiny(mon, info.otID);
        }
    }
}

void Cheats_OnEggHatched(struct Pokemon *mon)
{
    // The hatchling becomes the player's, and that's whose shiny it is
    u32 trainerID;
    if (mon != NULL && PortGame_PlayerTrainerID(&trainerID)) {
        MaybeMakeShiny(mon, trainerID);
    }
}
