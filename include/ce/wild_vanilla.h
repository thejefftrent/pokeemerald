#ifndef GUARD_CE_WILD_VANILLA_H
#define GUARD_CE_WILD_VANILLA_H

// Defines gCeVanillaWildWeights from the ENCOUNTER_CHANCE_* macros (cumulative thresholds)
// that tools/wild_encounters generates from wild_encounters.json into
// src/data/wild_encounters.h. Include it once, from src/wild_encounter.c, right after
// that file. The website reads this table to know what all-zero weights mean.

#include "ce/wild.h"

#define CE_CHANCE(m, i, j) (ENCOUNTER_CHANCE_##m##_SLOT_##j - ENCOUNTER_CHANCE_##m##_SLOT_##i)

const struct CeWildWeights gCeVanillaWildWeights =
{
    .land =
    {
        ENCOUNTER_CHANCE_LAND_MONS_SLOT_0, CE_CHANCE(LAND_MONS, 0, 1), CE_CHANCE(LAND_MONS, 1, 2),
        CE_CHANCE(LAND_MONS, 2, 3), CE_CHANCE(LAND_MONS, 3, 4), CE_CHANCE(LAND_MONS, 4, 5),
        CE_CHANCE(LAND_MONS, 5, 6), CE_CHANCE(LAND_MONS, 6, 7), CE_CHANCE(LAND_MONS, 7, 8),
        CE_CHANCE(LAND_MONS, 8, 9), CE_CHANCE(LAND_MONS, 9, 10), CE_CHANCE(LAND_MONS, 10, 11),
    },
    .water =
    {
        ENCOUNTER_CHANCE_WATER_MONS_SLOT_0, CE_CHANCE(WATER_MONS, 0, 1), CE_CHANCE(WATER_MONS, 1, 2),
        CE_CHANCE(WATER_MONS, 2, 3), CE_CHANCE(WATER_MONS, 3, 4),
    },
    .rockSmash =
    {
        ENCOUNTER_CHANCE_ROCK_SMASH_MONS_SLOT_0, CE_CHANCE(ROCK_SMASH_MONS, 0, 1), CE_CHANCE(ROCK_SMASH_MONS, 1, 2),
        CE_CHANCE(ROCK_SMASH_MONS, 2, 3), CE_CHANCE(ROCK_SMASH_MONS, 3, 4),
    },
    .fishing =
    {
        ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_SLOT_0, CE_CHANCE(FISHING_MONS_OLD_ROD, 0, 1),
        ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_SLOT_2, CE_CHANCE(FISHING_MONS_GOOD_ROD, 2, 3), CE_CHANCE(FISHING_MONS_GOOD_ROD, 3, 4),
        ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_SLOT_5, CE_CHANCE(FISHING_MONS_SUPER_ROD, 5, 6), CE_CHANCE(FISHING_MONS_SUPER_ROD, 6, 7),
        CE_CHANCE(FISHING_MONS_SUPER_ROD, 7, 8), CE_CHANCE(FISHING_MONS_SUPER_ROD, 8, 9),
    },
#ifdef ENCOUNTER_CHANCE_HIDDEN_MONS_TOTAL
    .hidden = { ENCOUNTER_CHANCE_HIDDEN_MONS_SLOT_0, CE_CHANCE(HIDDEN_MONS, 0, 1), CE_CHANCE(HIDDEN_MONS, 1, 2) },
#endif
    .rodFirstSlot =
    {
        0,
        NUM_FISHING_MONS_OLD_ROD_ENCOUNTER_SLOTS,
        NUM_FISHING_MONS_OLD_ROD_ENCOUNTER_SLOTS + NUM_FISHING_MONS_GOOD_ROD_ENCOUNTER_SLOTS,
    },
    .rodSlotCount =
    {
        NUM_FISHING_MONS_OLD_ROD_ENCOUNTER_SLOTS,
        NUM_FISHING_MONS_GOOD_ROD_ENCOUNTER_SLOTS,
        NUM_FISHING_MONS_SUPER_ROD_ENCOUNTER_SLOTS,
    },
};

// Vanilla draws each rod's slot from one roll over the largest rod total; that only
// matches a per-rod weighted pick while every rod's weights add up to the same total.
STATIC_ASSERT(ENCOUNTER_CHANCE_FISHING_MONS_OLD_ROD_TOTAL == ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_TOTAL
           && ENCOUNTER_CHANCE_FISHING_MONS_GOOD_ROD_TOTAL == ENCOUNTER_CHANCE_FISHING_MONS_SUPER_ROD_TOTAL, CeVanillaRodTotals)

#undef CE_CHANCE

#endif // GUARD_CE_WILD_VANILLA_H
