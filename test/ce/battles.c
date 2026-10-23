#include "global.h"
#include "test/battle.h"
#include "ce/test.h"

// CE battle tests. test/battle.h redefines PARAMETRIZE for battle tests, so they live apart
// from the function tests. The runner plays recorded battles, which game modes exempt like
// real ones (see modes_nuzlocke.c), so only QoL, caps, battle settings and the one mode rule
// that holds in every battle (no experience) are tested here.

WILD_BATTLE_TEST("CE: Party Exp. Share gives Exp. to Pokémon that didn't fight")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u32 startExp = gExperienceTables[gSpeciesInfo[SPECIES_WYNAUT].growthRate][40];

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    GIVEN {
        CeTest_SetQol(CE_QOL_EXP_SHARE_ALL, mode);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WYNAUT) { Level(40); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
        if (mode == CE_QOL_MODE_ON)
            MESSAGE("The rest of your team gained Exp. Points thanks to the Exp. Share!");
        else
            NOT MESSAGE("The rest of your team gained Exp. Points thanks to the Exp. Share!");
    } THEN {
        if (mode == CE_QOL_MODE_ON)
            EXPECT_GT(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_EXP), startExp);
        else
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_EXP), startExp);
    }
}

WILD_BATTLE_TEST("CE: Exp. on catch follows the QoL (with B_EXP_CATCH Gen 6+)")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u32 config = GEN_6;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; config = GEN_6; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  config = GEN_6; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  config = GEN_5; }

    GIVEN {
        WITH_CONFIG(B_EXP_CATCH, config);
        CeTest_SetQol(CE_QOL_CATCH_EXP, mode);
        PLAYER(SPECIES_WOBBUFFET) { Level(50); }
        OPPONENT(SPECIES_CATERPIE) { HP(1); }
    } WHEN {
        TURN { USE_ITEM(player, ITEM_ULTRA_BALL, WITH_RNG(RNG_BALLTHROW_SHAKE, 0)); }
    } SCENE {
        MESSAGE("You used Ultra Ball!");
        ANIMATION(ANIM_TYPE_SPECIAL, B_ANIM_BALL_THROW, player);
        if (mode == CE_QOL_MODE_ON && config >= GEN_6)
            EXPERIENCE_BAR(player);
        else
            NOT EXPERIENCE_BAR(player);
    }
}

WILD_BATTLE_TEST("CE: Battle Exp. stops at the level cap")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u32 level = 10, cap = 10;
    u32 capExp = gExperienceTables[gSpeciesInfo[SPECIES_WOBBUFFET].growthRate][10];

    PARAMETRIZE { mode = CE_QOL_MODE_ON;  level = 10; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  level = 9; }
    PARAMETRIZE { mode = CE_QOL_MODE_OFF; level = 10; }

    GIVEN {
        // Boss: test trainer 1 (RED), whose highest level is 5; cap = 5 + offset.
        CeTest_SetQol(CE_QOL_LEVEL_CAPS, mode);
        CE_TEST_CONFIG->body.levelCaps.bossCount = 1;
        CE_TEST_CONFIG->body.levelCaps.bosses[0] = 1;
        CE_TEST_CONFIG->body.levelCaps.levelOffset = cap - 5;
        PLAYER(SPECIES_WOBBUFFET) { Level(level); }
        OPPONENT(SPECIES_CATERPIE) { Level(50); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
    } THEN {
        if (mode == CE_QOL_MODE_ON)
        {
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_EXP), capExp);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), cap);
        }
        else
        {
            EXPECT_GT(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), cap);
        }
    }
}

WILD_BATTLE_TEST("CE: No-experience mode: no Exp. from battles, but EVs are still gained")
{
    struct CeModesConfig rules = {0};
    u32 noExp = CE_NO_EXP_OFF;
    u32 startExp = gExperienceTables[gSpeciesInfo[SPECIES_WOBBUFFET].growthRate][10];

    PARAMETRIZE { noExp = CE_NO_EXP_OFF; }
    PARAMETRIZE { noExp = CE_NO_EXP_BATTLE; }
    PARAMETRIZE { noExp = CE_NO_EXP_ALL; }

    GIVEN {
        ASSUME(gSpeciesInfo[SPECIES_CATERPIE].evYield_HP == 1);
        rules.noExp = noExp;
        CeTest_StartRun(&rules);
        PLAYER(SPECIES_WOBBUFFET) { Level(10); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
        if (noExp == CE_NO_EXP_OFF)
            EXPERIENCE_BAR(player);
        else
            NOT EXPERIENCE_BAR(player);
    } THEN {
        if (noExp == CE_NO_EXP_OFF)
            EXPECT_GT(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_EXP), startExp);
        else
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_EXP), startExp);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HP_EV), 1);
    }
}

WILD_BATTLE_TEST("CE: No-experience mode: no Exp. through the party Exp. Share either")
{
    struct CeModesConfig rules = { .noExp = CE_NO_EXP_BATTLE };
    u32 startExp = gExperienceTables[gSpeciesInfo[SPECIES_WYNAUT].growthRate][40];

    GIVEN {
        CeTest_SetQol(CE_QOL_EXP_SHARE_ALL, CE_QOL_MODE_ON);
        CeTest_StartRun(&rules);
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WYNAUT) { Level(40); }
        OPPONENT(SPECIES_CATERPIE) { Level(10); HP(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        MESSAGE("The wild Caterpie fainted!");
        NOT MESSAGE("The rest of your team gained Exp. Points thanks to the Exp. Share!");
    } THEN {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_EXP), startExp);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP_EV), 1);
    }
}

SINGLE_BATTLE_TEST("CE: Battle config: B_BURN_DAMAGE changes burn damage in battle")
{
    u32 gen = GEN_7, divisor = 16;

    PARAMETRIZE { gen = GEN_6; divisor = 8; }
    PARAMETRIZE { gen = GEN_7; divisor = 16; }

    GIVEN {
        WITH_CONFIG(B_BURN_DAMAGE, gen);
        PLAYER(SPECIES_WOBBUFFET) { Status1(STATUS1_BURN); }
        OPPONENT(SPECIES_WOBBUFFET);
    } WHEN {
        TURN {}
    } SCENE {
        HP_BAR(player, damage: GetMonData(&PLAYER_PARTY[0], MON_DATA_MAX_HP) / divisor);
    }
}
