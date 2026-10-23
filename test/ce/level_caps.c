#include "global.h"
#include "battle_setup.h"
#include "caps.h"
#include "data.h"
#include "pokemon.h"
#include "test/test.h"
#include "ce/level_caps.h"
#include "ce/test.h"

// Level caps (ce/level_caps.h). Test builds have expansion's test trainers in gTrainers
// (test/battle/trainer_control.party), so these use them as bosses.
#define BOSS_RED   1 // Charmander Lv. 5
#define BOSS_LEAF  2 // Bulbasaur Lv. 5
#define BOSS_TEST1 3 // Wobbuffet Lv. 67, Wobbuffet Lv. 5, Wynaut Lv. 5

static u32 HighestPartyLevel(u16 trainerId)
{
    const struct Trainer *trainer = GetTrainerStructFromId(trainerId);
    u32 i, ace = 0;

    for (i = 0; i < trainer->partySize; i++)
        ace = max(ace, trainer->party[i].lvl);
    return ace;
}

static void SetCaps(s8 levelOffset, u32 count, const u16 *bosses)
{
    CeTest_SetQol(CE_QOL_LEVEL_CAPS, CE_QOL_MODE_ON);
    CE_TEST_CONFIG->body.levelCaps.levelOffset = levelOffset;
    CE_TEST_CONFIG->body.levelCaps.bossCount = count;
    memcpy(CE_TEST_CONFIG->body.levelCaps.bosses, bosses, count * sizeof(bosses[0]));
    ClearTrainerFlag(BOSS_RED);
    ClearTrainerFlag(BOSS_LEAF);
    ClearTrainerFlag(BOSS_TEST1);
}

ASSUMPTIONS
{
    ASSUME(HighestPartyLevel(BOSS_RED) == 5);
    ASSUME(HighestPartyLevel(BOSS_LEAF) == 5);
    ASSUME(HighestPartyLevel(BOSS_TEST1) == 67);
}

TEST("CE: Level caps are off unless the QoL is on")
{
    static const u16 bosses[] = { BOSS_RED };

    SetCaps(0, ARRAY_COUNT(bosses), bosses);
    CeTest_SetQol(CE_QOL_LEVEL_CAPS, CE_QOL_MODE_OFF);
    EXPECT_EQ(GetCurrentLevelCap(), MAX_LEVEL);
    CeTest_SetQol(CE_QOL_LEVEL_CAPS, CE_QOL_MODE_ON);
    EXPECT_EQ(GetCurrentLevelCap(), 5);
}

TEST("CE: The level cap is the next boss's highest level in gTrainers, plus the offset")
{
    static const u16 bosses[] = { BOSS_RED, BOSS_TEST1 };
    s8 offset = 0;
    u16 next;

    PARAMETRIZE { offset = 0; }
    PARAMETRIZE { offset = 3; }
    PARAMETRIZE { offset = -2; }

    SetCaps(offset, ARRAY_COUNT(bosses), bosses);
    EXPECT_EQ(CeLevelCap_Compute(&next), HighestPartyLevel(BOSS_RED) + offset);
    EXPECT_EQ(next, BOSS_RED);
    SetTrainerFlag(BOSS_RED);
    EXPECT_EQ(CeLevelCap_Compute(&next), HighestPartyLevel(BOSS_TEST1) + offset);
    EXPECT_EQ(next, BOSS_TEST1);
    SetTrainerFlag(BOSS_TEST1);
    EXPECT_EQ(CeLevelCap_Compute(&next), MAX_LEVEL);
    EXPECT_EQ(next, TRAINER_NONE);
}

TEST("CE: The level cap never goes down")
{
    // The strongest boss comes first: beating it must not drop the cap to the weaker ones'.
    static const u16 bosses[] = { BOSS_TEST1, BOSS_RED, BOSS_LEAF };

    SetCaps(0, ARRAY_COUNT(bosses), bosses);
    EXPECT_EQ(GetCurrentLevelCap(), 67);
    // Losing sets no trainer flag (only a win does), so the cap stays where it was.
    EXPECT_EQ(GetCurrentLevelCap(), 67);
    SetTrainerFlag(BOSS_TEST1);
    EXPECT_EQ(GetCurrentLevelCap(), 67);
    SetTrainerFlag(BOSS_RED);
    EXPECT_EQ(GetCurrentLevelCap(), 67);
    SetTrainerFlag(BOSS_LEAF);
    EXPECT_EQ(GetCurrentLevelCap(), MAX_LEVEL);
}

TEST("CE: Rare Candy is refused at the level cap")
{
    static const u16 bosses[] = { BOSS_RED };

    SetCaps(5, ARRAY_COUNT(bosses), bosses);
    EXPECT_EQ(GetCurrentLevelCap(), 10);
    EXPECT(!CeLevelCap_BlocksLevelItem(ITEM_RARE_CANDY, 9));
    EXPECT(CeLevelCap_BlocksLevelItem(ITEM_RARE_CANDY, 10));
    EXPECT(CeLevelCap_BlocksLevelItem(ITEM_RARE_CANDY, 11));
    CeTest_SetQol(CE_QOL_LEVEL_CAPS, CE_QOL_MODE_OFF);
    EXPECT(!CeLevelCap_BlocksLevelItem(ITEM_RARE_CANDY, 99));
}

TEST("CE: Exp. Candies stop at the level cap")
{
    static const u16 bosses[] = { BOSS_RED };
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u32 capExp;

    SetCaps(5, ARRAY_COUNT(bosses), bosses);
    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_WOBBUFFET, 8, 0, OTID_STRUCT_PLAYER_ID);
    capExp = gExperienceTables[gSpeciesInfo[SPECIES_WOBBUFFET].growthRate][10];
    EXPECT_EQ(ExecuteTableBasedItemEffect(mon, ITEM_EXP_CANDY_XL, 0, 0), FALSE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_EXP), capExp);
    EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), 10);
}

TEST("CE: Day Care levels stop at the cap, never below where they started")
{
    static const u16 bosses[] = { BOSS_RED };

    SetCaps(5, ARRAY_COUNT(bosses), bosses);
    EXPECT_EQ(CeLevelCap_ClampGainedLevel(8, 20), 10);
    EXPECT_EQ(CeLevelCap_ClampGainedLevel(8, 9), 9);
    EXPECT_EQ(CeLevelCap_ClampGainedLevel(12, 15), 12);
}

