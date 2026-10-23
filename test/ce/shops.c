#include "global.h"
#include "event_data.h"
#include "item.h"
#include "shop_criteria.h"
#include "test/test.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/test.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"

// QoL shops (ce/progression.h): town marts that add a feature's stock, and the sellers in the
// player's room.

static void SetLocation(u16 map)
{
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(map);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(map);
}

static u32 CountShopItems(const u16 *items)
{
    u32 count;

    for (count = 0; items[count] != ITEM_NONE; count++)
        ;
    return count;
}

static bool32 IsInShop(const u16 *items, u16 item)
{
    u32 i;

    for (i = 0; items[i] != ITEM_NONE; i++)
    {
        if (items[i] == item)
            return TRUE;
    }
    return FALSE;
}

TEST("CE: A town mart adds its QoL stock after its own while the feature is on")
{
    static const u16 sScriptList[] = { ITEM_POKE_BALL, ITEM_POTION, ITEM_NONE };
    enum CeQolFeature feature = CE_QOL_NATURE_MINTS;
    const u16 *stock = gCeShopNatureMints;
    u16 map = MAP_LAVARIDGE_TOWN_HERB_SHOP;
    bool32 sells = FALSE;
    const u16 *list = sScriptList;
    u16 count = 0;
    u32 i, extras;

    PARAMETRIZE { feature = CE_QOL_NATURE_MINTS;         stock = gCeShopNatureMints;        map = MAP_LAVARIDGE_TOWN_HERB_SHOP; sells = FALSE; }
    PARAMETRIZE { feature = CE_QOL_NATURE_MINTS;         stock = gCeShopNatureMints;        map = MAP_LAVARIDGE_TOWN_HERB_SHOP; sells = TRUE; }
    PARAMETRIZE { feature = CE_QOL_GEMS_PLATES_MEMORIES; stock = gCeShopGemsPlatesMemories; map = MAP_SLATEPORT_CITY_MART;      sells = FALSE; }
    PARAMETRIZE { feature = CE_QOL_GEMS_PLATES_MEMORIES; stock = gCeShopGemsPlatesMemories; map = MAP_SLATEPORT_CITY_MART;      sells = TRUE; }
    PARAMETRIZE { feature = CE_QOL_Z_CRYSTALS;           stock = gCeShopZCrystals;          map = MAP_FORTREE_CITY_MART;        sells = FALSE; }
    PARAMETRIZE { feature = CE_QOL_Z_CRYSTALS;           stock = gCeShopZCrystals;          map = MAP_FORTREE_CITY_MART;        sells = TRUE; }
    PARAMETRIZE { feature = CE_QOL_MEGA_STONES;          stock = gCeShopMegaStones;         map = MAP_MOSSDEEP_CITY_MART;       sells = FALSE; }
    PARAMETRIZE { feature = CE_QOL_MEGA_STONES;          stock = gCeShopMegaStones;         map = MAP_MOSSDEEP_CITY_MART;       sells = TRUE; }

    // The config copy lives for the whole test, so every run sets every shop feature.
    CeTest_SetQol(CE_QOL_NATURE_MINTS, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_TRADE_EVOS_WITHOUT_TRADE, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_GEMS_PLATES_MEMORIES, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_Z_CRYSTALS, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_MEGA_STONES, CE_QOL_MODE_OFF);
    CeTest_SetQol(feature, sells ? CE_QOL_MODE_ON : CE_QOL_MODE_OFF);
    SetLocation(map);
    CeProgression_BuildShopItemList(&list, &count);
    extras = sells ? CountShopItems(stock) : 0;
    EXPECT_EQ(count, 2 + extras);
    EXPECT_EQ(list[0], ITEM_POKE_BALL);
    EXPECT_EQ(list[1], ITEM_POTION);
    for (i = 0; i < extras; i++)
        EXPECT_EQ(list[2 + i], stock[i]);
    EXPECT_EQ(list[count], ITEM_NONE);
    TryFreeDynamicShopItemList(&list);
    EXPECT(list == sScriptList);
}

TEST("CE: Another town's mart adds none of a QoL shop's stock")
{
    static const u16 sScriptList[] = { ITEM_POKE_BALL, ITEM_NONE };
    const u16 *list = sScriptList;
    u16 count = 0;

    CeTest_SetQol(CE_QOL_NATURE_MINTS, CE_QOL_MODE_ON);
    CeTest_SetQol(CE_QOL_TRADE_EVOS_WITHOUT_TRADE, CE_QOL_MODE_ON);
    CeTest_SetQol(CE_QOL_GEMS_PLATES_MEMORIES, CE_QOL_MODE_ON);
    CeTest_SetQol(CE_QOL_Z_CRYSTALS, CE_QOL_MODE_ON);
    CeTest_SetQol(CE_QOL_MEGA_STONES, CE_QOL_MODE_ON);
    SetLocation(MAP_OLDALE_TOWN_MART);
    CeProgression_BuildShopItemList(&list, &count);
    EXPECT_EQ(count, 1);
    TryFreeDynamicShopItemList(&list);
}

TEST("CE: The Mega Stone and Z-Crystal stock is every one of them, after the ring that unlocks it")
{
    u32 i;

    EXPECT_EQ(gCeShopMegaStones[0], ITEM_MEGA_RING);
    EXPECT_EQ(gCeShopZCrystals[0], ITEM_Z_POWER_RING);
    for (i = 1; gCeShopMegaStones[i] != ITEM_NONE; i++)
        EXPECT_EQ(GetItemHoldEffect(gCeShopMegaStones[i]), HOLD_EFFECT_MEGA_STONE);
    for (i = 1; gCeShopZCrystals[i] != ITEM_NONE; i++)
        EXPECT_EQ(GetItemHoldEffect(gCeShopZCrystals[i]), HOLD_EFFECT_Z_CRYSTAL);
    for (i = ITEM_NONE + 1; i < ITEMS_COUNT; i++)
    {
        enum HoldEffect holdEffect = GetItemHoldEffect(i);

        if (holdEffect == HOLD_EFFECT_MEGA_STONE)
            EXPECT(IsInShop(gCeShopMegaStones, i));
        else if (holdEffect == HOLD_EFFECT_Z_CRYSTAL)
            EXPECT(IsInShop(gCeShopZCrystals, i));
        else if (holdEffect == HOLD_EFFECT_GEMS || holdEffect == HOLD_EFFECT_PLATE || holdEffect == HOLD_EFFECT_MEMORY)
            EXPECT(IsInShop(gCeShopGemsPlatesMemories, i));
    }
}

TEST("CE: Room sellers stand in the player's own bedroom while Room sellers and their feature are on")
{
    u8 gender = MALE;
    u16 map = MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F;
    enum CeQolMode room = CE_QOL_MODE_OFF, mints = CE_QOL_MODE_OFF;
    bool32 open = FALSE;

    PARAMETRIZE { gender = MALE;   map = MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F; room = CE_QOL_MODE_ON;  mints = CE_QOL_MODE_ON;  open = TRUE; }
    PARAMETRIZE { gender = FEMALE; map = MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F;     room = CE_QOL_MODE_ON;  mints = CE_QOL_MODE_ON;  open = TRUE; }
    // The other house's bedroom is the rival's.
    PARAMETRIZE { gender = FEMALE; map = MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F; room = CE_QOL_MODE_ON;  mints = CE_QOL_MODE_ON;  open = FALSE; }
    PARAMETRIZE { gender = MALE;   map = MAP_LITTLEROOT_TOWN_MAYS_HOUSE_2F;     room = CE_QOL_MODE_ON;  mints = CE_QOL_MODE_ON;  open = FALSE; }
    PARAMETRIZE { gender = MALE;   map = MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F; room = CE_QOL_MODE_OFF; mints = CE_QOL_MODE_ON;  open = FALSE; }
    PARAMETRIZE { gender = MALE;   map = MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F; room = CE_QOL_MODE_ON;  mints = CE_QOL_MODE_OFF; open = FALSE; }

    gSaveBlock2Ptr->playerGender = gender;
    SetLocation(map);
    CeTest_SetQol(CE_QOL_ROOM_SELLERS, room);
    CeTest_SetQol(CE_QOL_NATURE_MINTS, mints);
    CeTest_SetQol(CE_QOL_MEGA_STONES, CE_QOL_MODE_OFF);
    FlagClear(FLAG_TEMP_5);
    FlagClear(FLAG_TEMP_7);
    CeProgression_UpdateRoomSellers();
    // The mint seller's hide flag (FLAG_TEMP_5 in map.json) is clear only while it's open.
    EXPECT_EQ(FlagGet(FLAG_TEMP_5), !open);
    // Mega Stones are off, so their seller is hidden either way.
    EXPECT(FlagGet(FLAG_TEMP_7));
}
