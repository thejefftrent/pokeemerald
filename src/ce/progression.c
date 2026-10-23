#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "field_specials.h"
#include "item.h"
#include "malloc.h"
#include "shop_criteria.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "constants/items.h"
#include "constants/maps.h"

bool32 CeProgression_IsReusableTm(enum Item itemId)
{
    // HMs are important in the item data already; the TMs are I_REUSABLE_TMS (FALSE) there.
    return GetItemPocket(itemId) == POCKET_TM_HM && CeQol_IsOn(CE_QOL_REUSABLE_TMS);
}

// The held items that a trade evolution needs, i.e. the EVO_HELD_ITEM_FIELD_FUNC items of
// src/data/items.h without Oval Stone and Razor Claw/Fang, whose evolutions need no trade.
bool32 CeProgression_IsBagEvoItem(enum Item itemId)
{
    switch (itemId)
    {
    case ITEM_KINGS_ROCK:     // Poliwhirl, Slowpoke
    case ITEM_METAL_COAT:     // Onix, Scyther
    case ITEM_DRAGON_SCALE:   // Seadra
    case ITEM_UPGRADE:        // Porygon
    case ITEM_DEEP_SEA_TOOTH: // Clamperl
    case ITEM_DEEP_SEA_SCALE: // Clamperl
    case ITEM_PROTECTOR:      // Rhydon
    case ITEM_ELECTIRIZER:    // Electabuzz
    case ITEM_MAGMARIZER:     // Magmar
    case ITEM_DUBIOUS_DISC:   // Porygon2
    case ITEM_REAPER_CLOTH:   // Dusclops
    case ITEM_PRISM_SCALE:    // Feebas
    case ITEM_WHIPPED_DREAM:  // Swirlix
    case ITEM_SACHET:         // Spritzee
        // Not in the Battle Pyramid: RestorePyramidPlayerParty matches mons by species.
        return CeQol_IsOn(CE_QOL_TRADE_EVOS_WITHOUT_TRADE) && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE;
    default:
        return FALSE;
    }
}

bool32 CeProgression_InFrontierChallenge(void)
{
    // The Pike, the Pyramid and the Tower multi partner room put Sketched moves back after
    // each battle (ResetSketchedMoves / RestorePyramidPlayerParty), so changing moves there
    // turns the new move into Sketch for good.
    return InBattlePike() || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InMultiPartnerRoom();
}

static const u16 sNatureMints[] =
{
    ITEM_LONELY_MINT, ITEM_ADAMANT_MINT, ITEM_NAUGHTY_MINT, ITEM_BRAVE_MINT, ITEM_BOLD_MINT,
    ITEM_IMPISH_MINT, ITEM_LAX_MINT, ITEM_RELAXED_MINT, ITEM_MODEST_MINT, ITEM_MILD_MINT,
    ITEM_RASH_MINT, ITEM_QUIET_MINT, ITEM_CALM_MINT, ITEM_GENTLE_MINT, ITEM_CAREFUL_MINT,
    ITEM_SASSY_MINT, ITEM_TIMID_MINT, ITEM_HASTY_MINT, ITEM_JOLLY_MINT, ITEM_NAIVE_MINT,
    ITEM_SERIOUS_MINT,
};

// Kadabra, Machoke, Graveler and Haunter (and later trade evolutions) evolve with it.
static const u16 sLinkingCord[] = { ITEM_LINKING_CORD };

struct CeMartExtras
{
    u16 map;      // MAP_*: the mart's map; each map listed here has one clerk
    u8 feature;   // enum CeQolFeature
    u8 count;
    const u16 *items;
};

static const struct CeMartExtras sMartExtras[] =
{
    { MAP_LAVARIDGE_TOWN_HERB_SHOP, CE_QOL_NATURE_MINTS,             ARRAY_COUNT(sNatureMints), sNatureMints },
    { MAP_MAUVILLE_CITY_MART,       CE_QOL_TRADE_EVOS_WITHOUT_TRADE, ARRAY_COUNT(sLinkingCord), sLinkingCord },
};

static bool32 IsMartExtrasHere(const struct CeMartExtras *extras)
{
    return gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(extras->map)
        && gSaveBlock1Ptr->location.mapNum == MAP_NUM(extras->map)
        && CeQol_IsOn(extras->feature);
}

static bool32 IsInList(const u16 *list, u32 count, u16 item)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (list[i] == item)
            return TRUE;
    }
    return FALSE;
}

void CeProgression_BuildShopItemList(const u16 **itemList, u16 *itemCount)
{
    u32 i, j, count, extraCount = 0;
    u16 *list;

    // This runs on every BUY of one clerk conversation. Leaving the buy menu restores the
    // script's list but not its count, which the extras below enlarged, so recount it the
    // way SetShopItemsForSale does before upstream reads that many entries.
    for (count = 0; (*itemList)[count] != ITEM_NONE; count++)
        ;
    *itemCount = count;
    TryBuildDynamicShopItemList(itemList, itemCount);

    for (i = 0; i < ARRAY_COUNT(sMartExtras); i++)
    {
        if (IsMartExtrasHere(&sMartExtras[i]))
            extraCount += sMartExtras[i].count;
    }
    if (extraCount == 0)
        return;

    // Same shape as upstream's dynamic list (heap, ITEM_NONE-terminated), so
    // TryFreeDynamicShopItemList frees this one and restores the script's list.
    list = AllocZeroed((*itemCount + extraCount + 1) * sizeof(u16));
    if (list == NULL)
        return;

    count = *itemCount;
    for (i = 0; i < count; i++)
        list[i] = (*itemList)[i];

    for (i = 0; i < ARRAY_COUNT(sMartExtras); i++)
    {
        if (!IsMartExtrasHere(&sMartExtras[i]))
            continue;
        for (j = 0; j < sMartExtras[i].count; j++)
        {
            u16 item = sMartExtras[i].items[j];

            if (!IsInList(list, count, item) && IsItemShopCriteriaFulfilled(item))
                list[count++] = item;
        }
    }
    list[count] = ITEM_NONE;

    Free((u16 *)*itemList);
    *itemList = list;
    *itemCount = count;
}

#define CE_FAST_EGG_CYCLE_DIVISOR 4

u32 CeProgression_EggCycleLength(u32 steps)
{
    if (CeQol_IsOn(CE_QOL_FAST_EGG_HATCHING))
        return steps / CE_FAST_EGG_CYCLE_DIVISOR;
    return steps;
}
