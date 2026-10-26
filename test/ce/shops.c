#include "global.h"
#include "event_data.h"
#include "item.h"
#include "item_use.h"
#include "shop_criteria.h"
#include "test/test.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/test.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/maps.h"

// QoL shops (ce/progression.h): town marts that add a feature's stock, and the sellers upstairs
// in every Pokémon Center.

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
    CeTest_SetQol(CE_QOL_BATTLE_ITEMS, CE_QOL_MODE_OFF);
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
    CeTest_SetQol(CE_QOL_BATTLE_ITEMS, CE_QOL_MODE_ON);
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

TEST("CE: No town mart sells battle items, only their Pokémon Center seller")
{
    static const u16 sScriptList[] = { ITEM_POKE_BALL, ITEM_NONE };
    u16 map = MAP_OLDALE_TOWN_MART;
    const u16 *list = sScriptList;
    u16 count = 0;

    PARAMETRIZE { map = MAP_LAVARIDGE_TOWN_HERB_SHOP; }
    PARAMETRIZE { map = MAP_MAUVILLE_CITY_MART; }
    PARAMETRIZE { map = MAP_SLATEPORT_CITY_MART; }
    PARAMETRIZE { map = MAP_FORTREE_CITY_MART; }
    PARAMETRIZE { map = MAP_MOSSDEEP_CITY_MART; }
    PARAMETRIZE { map = MAP_SOOTOPOLIS_CITY_MART; }
    PARAMETRIZE { map = MAP_OLDALE_TOWN_MART; }

    CeTest_SetQol(CE_QOL_NATURE_MINTS, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_TRADE_EVOS_WITHOUT_TRADE, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_GEMS_PLATES_MEMORIES, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_Z_CRYSTALS, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_MEGA_STONES, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_BATTLE_ITEMS, CE_QOL_MODE_ON);
    SetLocation(map);
    CeProgression_BuildShopItemList(&list, &count);
    EXPECT_EQ(count, 1);
    TryFreeDynamicShopItemList(&list);
}

TEST("CE: PC sellers stand on a Pokémon Center 2F while their feature (and Center sellers, for a shop seller) is on")
{
    bool32 onPc = FALSE;
    enum CeQolMode center = CE_QOL_MODE_OFF, training = CE_QOL_MODE_OFF, battle = CE_QOL_MODE_OFF;
    bool32 trainingOpen, battleOpen;

    // onPc FALSE is anywhere else, such as the bedrooms that keep the old sellers as placeholders.
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_ON;  battle = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_OFF; training = CE_QOL_MODE_OFF; battle = CE_QOL_MODE_OFF; }

    CeTest_SetQol(CE_QOL_ROOM_SELLERS, center);
    CeTest_SetQol(CE_QOL_NATURE_MINTS, training);
    CeTest_SetQol(CE_QOL_BATTLE_ITEMS, battle);
    CeTest_SetQol(CE_QOL_MEGA_STONES, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_TUTOR_SELLER, CE_QOL_MODE_OFF);
    CeTest_SetQol(CE_QOL_WONDER_TRADE, CE_QOL_MODE_OFF);
    FlagClear(FLAG_TEMP_5);
    FlagClear(FLAG_TEMP_7);
    FlagClear(FLAG_TEMP_A);
    FlagClear(FLAG_TEMP_B);
    FlagClear(FLAG_TEMP_C);
    CeProgression_UpdatePcSellers(onPc);

    // A shop seller that a town mart also stocks needs Center sellers too; the battle items
    // seller, found nowhere else, needs only its own feature.
    trainingOpen = onPc && center == CE_QOL_MODE_ON && training == CE_QOL_MODE_ON;
    battleOpen = onPc && battle == CE_QOL_MODE_ON;
    EXPECT_EQ(FlagGet(FLAG_TEMP_5), !trainingOpen);
    EXPECT_EQ(FlagGet(FLAG_TEMP_A), !battleOpen);
    // Mega Stones are off, so their seller is hidden either way.
    EXPECT(FlagGet(FLAG_TEMP_7));
    // The move tutor and Wonder Trade are off.
    EXPECT(FlagGet(FLAG_TEMP_B));
    EXPECT(FlagGet(FLAG_TEMP_C));
}

TEST("CE: The Pokémon Center move tutor stands upstairs while its feature is on, with or without Center sellers")
{
    bool32 onPc = FALSE;
    enum CeQolMode center = CE_QOL_MODE_OFF, tutor = CE_QOL_MODE_OFF;

    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  tutor = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; tutor = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  tutor = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; tutor = CE_QOL_MODE_PLAYER_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; tutor = CE_QOL_MODE_PLAYER_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  tutor = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_OFF; tutor = CE_QOL_MODE_ON;  }

    gSaveBlock2Ptr->pokedex.ceQolChosen[1] = gSaveBlock2Ptr->pokedex.ceQolValue[1] = 0;
    CeTest_SetQol(CE_QOL_ROOM_SELLERS, center);
    CeTest_SetQol(CE_QOL_TUTOR_SELLER, tutor);
    FlagClear(FLAG_TEMP_B);
    CeProgression_UpdatePcSellers(onPc);
    EXPECT_EQ(FlagGet(FLAG_TEMP_B), !(onPc && (tutor == CE_QOL_MODE_ON || tutor == CE_QOL_MODE_PLAYER_ON)));
}

TEST("CE: The Wonder Trade attendant stands upstairs while its feature is on, with or without Center sellers")
{
    bool32 onPc = FALSE;
    enum CeQolMode center = CE_QOL_MODE_OFF, trade = CE_QOL_MODE_OFF;

    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  trade = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; trade = CE_QOL_MODE_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_ON;  trade = CE_QOL_MODE_OFF; }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; trade = CE_QOL_MODE_PLAYER_ON;  }
    PARAMETRIZE { onPc = TRUE;  center = CE_QOL_MODE_OFF; trade = CE_QOL_MODE_PLAYER_OFF; }
    PARAMETRIZE { onPc = FALSE; center = CE_QOL_MODE_ON;  trade = CE_QOL_MODE_ON;  }

    gSaveBlock2Ptr->pokedex.ceQolChosen[1] = gSaveBlock2Ptr->pokedex.ceQolValue[1] = 0;
    CeTest_SetQol(CE_QOL_ROOM_SELLERS, center);
    CeTest_SetQol(CE_QOL_WONDER_TRADE, trade);
    FlagClear(FLAG_TEMP_C);
    CeProgression_UpdatePcSellers(onPc);
    // The run's rules can hide it too: test/ce/wonder_trade.c.
    EXPECT_EQ(FlagGet(FLAG_TEMP_C), !(onPc && (trade == CE_QOL_MODE_ON || trade == CE_QOL_MODE_PLAYER_ON)));
}

TEST("CE: The PC seller table hides with FLAG_TEMP_5 onwards, in the maps' object order")
{
    static const u8 sFeatures[] =
    {
        CE_QOL_NATURE_MINTS, CE_QOL_TRADE_EVOS_WITHOUT_TRADE, CE_QOL_MEGA_STONES, CE_QOL_Z_CRYSTALS,
        CE_QOL_GEMS_PLATES_MEMORIES, CE_QOL_BATTLE_ITEMS, CE_QOL_TUTOR_SELLER, CE_QOL_WONDER_TRADE,
    };
    u32 i, feature;
    u16 flag;
    bool32 needsCenterSellers;

    for (i = 0; CeProgression_GetPcSeller(i, &flag, &feature, &needsCenterSellers); i++)
    {
        EXPECT_LT(i, ARRAY_COUNT(sFeatures));
        EXPECT_EQ(flag, FLAG_TEMP_5 + i);
        EXPECT_EQ(feature, sFeatures[i]);
        EXPECT_EQ(needsCenterSellers, i < 5);
        // Never the follower's flag.
        EXPECT_NE(flag, FLAG_TEMP_HIDE_FOLLOWER);
    }
    EXPECT_EQ(i, ARRAY_COUNT(sFeatures));
}

TEST("CE: The training stock is the mints, Ability Capsule and Patch, the vitamins and every Power item")
{
    static const u16 sOthers[] =
    {
        ITEM_ABILITY_CAPSULE, ITEM_ABILITY_PATCH,
        ITEM_HP_UP, ITEM_PROTEIN, ITEM_IRON, ITEM_CALCIUM, ITEM_ZINC, ITEM_CARBOS,
    };
    u32 i, mints = CountShopItems(gCeShopNatureMints), powerItems = 0;

    EXPECT_EQ(mints, 21);
    for (i = 0; i < mints; i++)
        EXPECT_EQ(gCeShopTrainingItems[i], gCeShopNatureMints[i]);
    for (i = 0; i < ARRAY_COUNT(sOthers); i++)
        EXPECT(IsInShop(gCeShopTrainingItems, sOthers[i]));
    for (i = ITEM_NONE + 1; i < ITEMS_COUNT; i++)
    {
        if (GetItemFieldFunc(i) == ItemUseOutOfBattle_Mint)
            EXPECT(IsInShop(gCeShopNatureMints, i));
        if (GetItemHoldEffect(i) == HOLD_EFFECT_POWER_ITEM)
        {
            EXPECT(IsInShop(gCeShopTrainingItems, i));
            powerItems++;
        }
    }
    EXPECT_EQ(powerItems, 6);
    EXPECT_EQ(CountShopItems(gCeShopTrainingItems), mints + ARRAY_COUNT(sOthers) + powerItems);
    for (i = 0; gCeShopTrainingItems[i] != ITEM_NONE; i++)
        EXPECT_GT(GetItemPrice(gCeShopTrainingItems[i]), 0);
}

TEST("CE: Every battle item has a hold effect and a price, and no other QoL shop sells it")
{
    static const u16 *const sOtherShops[] =
    {
        gCeShopTrainingItems, gCeShopEvolutionItems, gCeShopMegaStones, gCeShopZCrystals,
        gCeShopGemsPlatesMemories,
    };
    u32 i, j, k;

    EXPECT_EQ(CountShopItems(gCeShopBattleItems), 55);
    for (i = 0; gCeShopBattleItems[i] != ITEM_NONE; i++)
    {
        u16 item = gCeShopBattleItems[i];

        EXPECT_NE(GetItemHoldEffect(item), HOLD_EFFECT_NONE);
        EXPECT_GT(GetItemPrice(item), 0);
        for (j = 0; j < ARRAY_COUNT(sOtherShops); j++)
            EXPECT(!IsInShop(sOtherShops[j], item));
        for (k = 0; k < i; k++)
            EXPECT_NE(gCeShopBattleItems[k], item);
    }
}
