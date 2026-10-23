#include "global.h"
#include "event_data.h"
#include "random.h"
#include "wild_encounter.h"
#include "constants/items.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "ce/wild.h"

// Wild encounters: the header array in force, the tables of each probe map as the game
// finds them (GetCurrentMapWildMonHeaderId), and a slot histogram per method drawn with the
// game's own slot pickers, which the canary checks against the configured weights.

#define CE_WILD_DRAWS 10000
#define CE_WILD_SEED  0xCE5EED
#define CE_WILD_METHODS 5 // enum WildPokemonArea, LAND to HIDDEN

static const char *const sSlot[] = { "h0", "h1", "h2", "h3", "h4", "h5", "h6", "h7", "h8", "h9", "h10", "h11" };
static const char *const sSpecies[] = { "sp0", "sp1", "sp2", "sp3", "sp4", "sp5", "sp6", "sp7", "sp8", "sp9", "sp10", "sp11" };
static const char *const sLevels[] = { "lv0", "lv1", "lv2", "lv3", "lv4", "lv5", "lv6", "lv7", "lv8", "lv9", "lv10", "lv11" };
static const u8 sSlotCount[CE_WILD_METHODS] = { CE_WILD_LAND_SLOTS, CE_WILD_WATER_SLOTS, CE_WILD_ROCK_SLOTS, CE_WILD_FISHING_SLOTS, CE_WILD_HIDDEN_SLOTS };

STATIC_ASSERT(ARRAY_COUNT(sSlot) == CE_WILD_LAND_SLOTS && CE_WILD_LAND_SLOTS >= CE_WILD_FISHING_SLOTS, CeWildProbeNames)

static u32 DrawSlot(u32 area, u32 rod)
{
    switch (area)
    {
    case WILD_AREA_LAND:
        return ChooseWildMonIndex_Land();
    case WILD_AREA_WATER:
        return ChooseWildMonIndex_Water();
    case WILD_AREA_ROCKS:
        return ChooseWildMonIndex_Rocks();
    case WILD_AREA_FISHING:
        return CeWild_ChooseFishingSlot(rod);
    default:
        return ChooseHiddenMonIndex();
    }
}

static void ProbeHistogram(u32 area, u32 rod)
{
    u16 counts[CE_WILD_LAND_SLOTS] = {0};
    u32 i, slot, none = 0, slots = sSlotCount[area];

    for (i = 0; i < CE_WILD_DRAWS; i++)
    {
        slot = DrawSlot(area, rod);
        if (slot < slots)
            counts[slot]++;
        else
            none++;
    }
    CeSelfTest_LineStart("wildHist");
    CeSelfTest_PutField("method", area);
    CeSelfTest_PutField("rod", rod);
    CeSelfTest_PutField("draws", CE_WILD_DRAWS);
    for (i = 0; i < slots; i++)
        CeSelfTest_PutField(sSlot[i], counts[i]);
    CeSelfTest_PutField("none", none);
    CeSelfTest_LineEmit();
}

static const struct WildPokemonInfo *GetInfo(const struct WildEncounterTypes *types, u32 area)
{
    switch (area)
    {
    case WILD_AREA_LAND:
        return types->landMonsInfo;
    case WILD_AREA_WATER:
        return types->waterMonsInfo;
    case WILD_AREA_ROCKS:
        return types->rockSmashMonsInfo;
    case WILD_AREA_FISHING:
        return types->fishingMonsInfo;
    default:
        return types->hiddenMonsInfo;
    }
}

static void ProbeMap(u16 map)
{
    struct WarpData saved = gSaveBlock1Ptr->location;
    const struct WildPokemonHeader *header;
    const struct WildPokemonInfo *info;
    u32 headerId, time, area, i;

    // Look the map up the way the game does when the player stands on it.
    gSaveBlock1Ptr->location.mapGroup = map >> 8;
    gSaveBlock1Ptr->location.mapNum = map & 0xFF;
    headerId = GetCurrentMapWildMonHeaderId();
    gSaveBlock1Ptr->location = saved;

    CeSelfTest_LineStart("wildMap");
    CeSelfTest_PutField("map", map);
    CeSelfTest_PutField("header", headerId);
    CeSelfTest_LineEmit();
    if (headerId == HEADER_NONE)
        return;

    header = &gWildMonHeaders[headerId];
    for (time = 0; time < TIMES_OF_DAY_COUNT; time++)
    {
        for (area = 0; area < CE_WILD_METHODS; area++)
        {
            info = GetInfo(&header->encounterTypes[time], area);
            if (info == NULL)
                continue;

            CeSelfTest_LineStart("wildTable");
            CeSelfTest_PutField("map", map);
            CeSelfTest_PutField("time", time);
            CeSelfTest_PutField("method", area);
            CeSelfTest_PutField("rate", info->encounterRate);
            for (i = 0; i < sSlotCount[area]; i++)
                CeSelfTest_PutField(sSpecies[i], info->wildPokemon[i].species);
            CeSelfTest_LineEmit();

            CeSelfTest_LineStart("wildLevels");
            CeSelfTest_PutField("map", map);
            CeSelfTest_PutField("time", time);
            CeSelfTest_PutField("method", area);
            for (i = 0; i < sSlotCount[area]; i++)
                CeSelfTest_PutField(sLevels[i], info->wildPokemon[i].minLevel | (info->wildPokemon[i].maxLevel << 8));
            CeSelfTest_LineEmit();
        }
    }
}

void CeSelfTest_ProbeWild(void)
{
    const struct CeWildConfig *config = &gCeRomConfig.body.wild;
    rng_value_t saved;
    u32 i, rod;

    CeSelfTest_LineStart("wild");
    CeSelfTest_PutField("headers", (u32)gWildMonHeaders);
    CeSelfTest_PutField("probeMaps", config->probeMapCount);
    CeSelfTest_LineEmit();

    for (i = 0; i < config->probeMapCount && i < CE_WILD_PROBE_MAPS; i++)
        ProbeMap(config->probeMaps[i]);

    // No lure, so the pickers don't mirror slots. A fixed seed makes the histograms the
    // same on every run of the same ROM; the other probes get the RNG back as it was.
    VarSet(VAR_REPEL_STEP_COUNT, 0);
    saved = gRngValue;
    SeedRng(CE_WILD_SEED);
    ProbeHistogram(WILD_AREA_LAND, 0);
    ProbeHistogram(WILD_AREA_WATER, 0);
    ProbeHistogram(WILD_AREA_ROCKS, 0);
    for (rod = OLD_ROD; rod <= SUPER_ROD; rod++)
        ProbeHistogram(WILD_AREA_FISHING, rod);
    ProbeHistogram(WILD_AREA_HIDDEN, 0);
    gRngValue = saved;
}
