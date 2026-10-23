#include "global.h"
#include "random.h"
#include "wild_encounter.h"
#include "constants/items.h"
#include "ce/rom_config.h"
#include "ce/wild.h"

STATIC_ASSERT(CE_WILD_LAND_SLOTS == NUM_LAND_MONS_ENCOUNTER_SLOTS, CeWildLandSlots)
STATIC_ASSERT(CE_WILD_WATER_SLOTS == NUM_WATER_MONS_ENCOUNTER_SLOTS, CeWildWaterSlots)
STATIC_ASSERT(CE_WILD_ROCK_SLOTS == NUM_ROCK_SMASH_MONS_ENCOUNTER_SLOTS, CeWildRockSlots)
STATIC_ASSERT(CE_WILD_FISHING_SLOTS == NUM_FISHING_MONS_ENCOUNTER_SLOTS, CeWildFishingSlots)
STATIC_ASSERT(CE_WILD_HIDDEN_SLOTS == NUM_HIDDEN_MONS_ENCOUNTER_SLOTS, CeWildHiddenSlots)
STATIC_ASSERT(OLD_ROD == 0 && GOOD_ROD == 1 && SUPER_ROD == 2, CeWildRodIds)

static bool32 AllZero(const u8 *values, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        if (values[i] != 0)
            return FALSE;
    }
    return TRUE;
}

// The config's weights for one method, or vanilla's when the config leaves them all zero.
static const u8 *Weights(const u8 *custom, const u8 *vanilla, u32 count, bool32 *isVanilla)
{
    *isVanilla = AllZero(custom, count);
    return *isVanilla ? vanilla : custom;
}

// Vanilla rolls Random() % total and walks cumulative thresholds; this does the same, so
// the base ROM picks the same slot from the same RNG state. Custom weights roll the full
// 32 bits, so a total that doesn't divide 65536 doesn't bias the pick.
static u32 PickWeighted(const u8 *weights, u32 count, bool32 isVanilla)
{
    u32 i, total = 0, roll;

    for (i = 0; i < count; i++)
        total += weights[i];
    if (total == 0)
        return CE_WILD_NO_SLOT;

    roll = (isVanilla ? Random() : Random32()) % total;
    for (i = 0; i + 1 < count; i++)
    {
        if (roll < weights[i])
            return i;
        roll -= weights[i];
    }
    return count - 1;
}

u32 CeWild_ChooseSlot(u32 area)
{
    const struct CeWildWeights *custom = &gCeRomConfig.body.wild.weights;
    const u8 *weights;
    bool32 isVanilla;
    u32 count;

    switch (area)
    {
    default:
    case WILD_AREA_LAND:
        count = CE_WILD_LAND_SLOTS;
        weights = Weights(custom->land, gCeVanillaWildWeights.land, count, &isVanilla);
        break;
    case WILD_AREA_WATER:
        count = CE_WILD_WATER_SLOTS;
        weights = Weights(custom->water, gCeVanillaWildWeights.water, count, &isVanilla);
        break;
    case WILD_AREA_ROCKS:
        count = CE_WILD_ROCK_SLOTS;
        weights = Weights(custom->rockSmash, gCeVanillaWildWeights.rockSmash, count, &isVanilla);
        break;
    case WILD_AREA_HIDDEN:
        count = CE_WILD_HIDDEN_SLOTS;
        weights = Weights(custom->hidden, gCeVanillaWildWeights.hidden, count, &isVanilla);
        break;
    }
    return PickWeighted(weights, count, isVanilla);
}

// The rod's slot range, kept inside the fishing table whatever the config says.
static void GetRodRange(u32 rod, u32 *first, u32 *count)
{
    const struct CeWildWeights *custom = &gCeRomConfig.body.wild.weights;
    const struct CeWildWeights *ranges = AllZero(custom->rodSlotCount, CE_WILD_ROD_COUNT) ? &gCeVanillaWildWeights : custom;

    if (rod >= CE_WILD_ROD_COUNT)
        rod = OLD_ROD;
    *first = min(ranges->rodFirstSlot[rod], CE_WILD_FISHING_SLOTS - 1);
    *count = min(max(ranges->rodSlotCount[rod], 1), CE_WILD_FISHING_SLOTS - *first);
}

u32 CeWild_ChooseFishingSlot(u32 rod)
{
    const u8 *weights;
    bool32 isVanilla;
    u32 first, count, slot;

    GetRodRange(rod, &first, &count);
    weights = Weights(gCeRomConfig.body.wild.weights.fishing, gCeVanillaWildWeights.fishing, CE_WILD_FISHING_SLOTS, &isVanilla);
    slot = PickWeighted(&weights[first], count, isVanilla);
    return first + (slot == CE_WILD_NO_SLOT ? 0 : slot);
}

u32 CeWild_MirrorFishingSlot(u32 rod, u32 slot)
{
    u32 first, count;

    GetRodRange(rod, &first, &count);
    if (slot < first || slot >= first + count)
        return slot;
    return first + (first + count - 1 - slot);
}
