#ifndef GUARD_CE_WILD_H
#define GUARD_CE_WILD_H

// Wild encounter slot weights and the patchable wild header table.
//
// The game picks a wild slot by weight. Vanilla's weights are compiled into
// gCeVanillaWildWeights from wild_encounters.json; the website can replace them per
// method in gCeRomConfig.body.wild. A method whose weights are all zero uses vanilla's,
// and all-zero rod ranges use vanilla's, so the base ROM picks slots exactly like vanilla
// (same thresholds and the same Random() call).
//
// Every reader of gWildMonHeaders goes through the CE_PATCHABLE pointer gCeWildMonHeaders
// (see include/wild_encounter.h), so the website can write a longer header array into
// free space, which is how maps without encounters get them.

// Slot counts, checked against NUM_*_ENCOUNTER_SLOTS in src/ce/wild.c.
#define CE_WILD_LAND_SLOTS    12
#define CE_WILD_WATER_SLOTS   5
#define CE_WILD_ROCK_SLOTS    5
#define CE_WILD_FISHING_SLOTS 10
#define CE_WILD_HIDDEN_SLOTS  3
#define CE_WILD_ROD_COUNT     3 // OLD_ROD, GOOD_ROD, SUPER_ROD
#define CE_WILD_PROBE_MAPS    4

// Returned for a method with no weights at all (vanilla hidden mons).
#define CE_WILD_NO_SLOT 0xFF

struct CeWildWeights
{
    u8 land[CE_WILD_LAND_SLOTS];
    u8 water[CE_WILD_WATER_SLOTS];
    u8 rockSmash[CE_WILD_ROCK_SLOTS];
    u8 fishing[CE_WILD_FISHING_SLOTS];
    u8 hidden[CE_WILD_HIDDEN_SLOTS];
    // The fishing slots each rod draws from: fishing[rodFirstSlot[rod]] onwards, rodSlotCount[rod] of them.
    u8 rodFirstSlot[CE_WILD_ROD_COUNT];
    u8 rodSlotCount[CE_WILD_ROD_COUNT];
};

struct CeWildConfig
{
    struct CeWildWeights weights;
    // Self-test only: maps (MAP_GROUP << 8 | MAP_NUM) whose tables CeSelfTest_ProbeWild prints.
    u8 probeMapCount;
    u16 probeMaps[CE_WILD_PROBE_MAPS];
};

extern const struct CeWildWeights gCeVanillaWildWeights;

// For the website's check that a map has tiles a new table could use: how map blocks
// name metatiles and how metatile attributes hold a behavior, whose sTileBitAttributes
// entry has gCeTileFlags[0] set for encounter tiles and gCeTileFlags[1] for surfable ones.
struct CeWildTerrainInfo
{
    u16 metatileIdMask;
    u16 primaryMetatiles;
    u16 behaviorMask;
};

extern const struct CeWildTerrainInfo gCeWildTerrainInfo;
extern const u8 gCeTileFlags[2];

// A slot for WILD_AREA_LAND, _WATER, _ROCKS or _HIDDEN, by the weights in force.
// CE_WILD_NO_SLOT when the method has no weights (hidden mons in vanilla).
u32 CeWild_ChooseSlot(u32 area);

// A fishing slot for OLD_ROD, GOOD_ROD or SUPER_ROD, within that rod's slot range.
u32 CeWild_ChooseFishingSlot(u32 rod);

// The slot a lure swaps `slot` to: mirrored within the rod's slot range.
u32 CeWild_MirrorFishingSlot(u32 rod, u32 slot);

#endif // GUARD_CE_WILD_H
