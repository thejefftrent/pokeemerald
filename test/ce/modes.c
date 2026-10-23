#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "event_data.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "test/test.h"
#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/test.h"
#include "constants/battle.h"
#include "constants/region_map_sections.h"

// Game modes other than Nuzlocke (ce/modes.h): the save keeps the rules New Game copied.

extern const u8 CeEventScript_CenterHealRefused[];
extern const u8 CeEventScript_RestHealRefused[];

TEST("CE: Modes: New Game copies the rules and the config hash into the save")
{
    static const struct CeModesConfig rules = { .nuzlocke = TRUE, .monotype = TYPE_FIRE };
    static const struct CeModesConfig other = { .noRestHeal = TRUE };

    CE_TEST_CONFIG->hdr.configHash = 0x1234ABCD;
    CeTest_StartRun(&rules);
    EXPECT(CeModes_SaveHasRules());
    EXPECT_EQ(gSaveBlock2Ptr->pokedex.ceModes.configHash, 0x1234ABCD);
    EXPECT_EQ(CeModes_Rules()->monotype, TYPE_FIRE);

    // A re-patched ROM doesn't change the run in progress.
    CE_TEST_CONFIG->body.modes = other;
    EXPECT_EQ(CeModes_Rules()->monotype, TYPE_FIRE);
    EXPECT_EQ(CeModes_Rules()->noRestHeal, FALSE);
}

TEST("CE: Modes: a ROM without rules leaves the save without rules")
{
    static const struct CeModesConfig none = {0};

    CeTest_StartRun(&none);
    EXPECT(!CeModes_SaveHasRules());
    EXPECT_EQ(CeModes_PartyLimit(), PARTY_SIZE);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_OK);
}

TEST("CE: Monotype: only Pokémon of the run's type can be caught")
{
    static const struct CeModesConfig rules = { .monotype = TYPE_FIRE };

    ASSUME(GetSpeciesType(SPECIES_TORCHIC, 0) == TYPE_FIRE);
    ASSUME(GetSpeciesType(SPECIES_ZIGZAGOON, 0) == TYPE_NORMAL && GetSpeciesType(SPECIES_ZIGZAGOON, 1) == TYPE_NORMAL);
    CeTest_StartRun(&rules);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_TORCHIC, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_OK);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_MONOTYPE);
    // Statics too; nothing is caught from trainers or in exempt battles.
    EXPECT_EQ(CeModes_CatchRule(SPECIES_KYOGRE, FALSE, CE_AREA_NONE, CE_BATTLE_SCRIPTED), CE_CATCH_MONOTYPE);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, CE_AREA_NONE, CE_BATTLE_EXEMPT), CE_CATCH_OK);
    // An encounter that can't be caught never uses up a Nuzlocke area.
    EXPECT(!CeModes_CountsAsEncounter(SPECIES_ZIGZAGOON, FALSE));
}

TEST("CE: Single-Pokémon: only that family can be caught")
{
    static const struct CeModesConfig rules = { .onlySpecies = SPECIES_ZIGZAGOON };

    CeTest_StartRun(&rules);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_OK);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_SPECIES);
    // Families come from gCeFamilyRoot, which the website fills; empty, each species is its own.
    EXPECT_EQ(CeModes_FamilyRoot(SPECIES_LINOONE), SPECIES_LINOONE);
}

TEST("CE: Single-Pokémon party limit sends extras to the PC")
{
    struct CeModesConfig rules = {0};
    struct Pokemon mon;
    u32 limit = 0;

    PARAMETRIZE { limit = 0; }
    PARAMETRIZE { limit = 1; }
    PARAMETRIZE { limit = 2; }

    rules.partyLimit = limit;
    rules.onlySpecies = SPECIES_ZIGZAGOON;
    CeTest_StartRun(&rules);
    ZeroPlayerPartyMons();
    EXPECT_EQ(CeModes_PartyLimit(), limit == 0 ? PARTY_SIZE : limit);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
    CreateMon(&mon, SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GiveCapturedMonToPlayer(&mon), limit == 1 ? MON_GIVEN_TO_PC : MON_GIVEN_TO_PARTY);
    CreateMon(&mon, SPECIES_LINOONE, 5, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GiveCapturedMonToPlayer(&mon), limit != 0 ? MON_GIVEN_TO_PC : MON_GIVEN_TO_PARTY);
}

TEST("CE: No-heal: Pokémon Center and rest heals are refused by the rules")
{
    struct CeModesConfig rules = {0};
    struct ScriptContext ctx;
    bool32 center = FALSE, rest = FALSE;

    PARAMETRIZE { center = FALSE; rest = FALSE; }
    PARAMETRIZE { center = TRUE;  rest = FALSE; }
    PARAMETRIZE { center = FALSE; rest = TRUE; }

    rules.noCenterHeal = center;
    rules.noRestHeal = rest;
    rules.monotype = TYPE_NORMAL; // keep the rules non-empty in every run
    CeTest_StartRun(&rules);
    EXPECT_EQ(CeModes_CanHeal(CE_HEAL_CENTER), !center);
    EXPECT_EQ(CeModes_CanHeal(CE_HEAL_REST), !rest);

    // The script natives jump to the refusal scripts instead of healing.
    memset(&ctx, 0, sizeof(ctx));
    CeModes_ScriptCenterHealGate(&ctx);
    EXPECT(ctx.scriptPtr == (center ? CeEventScript_CenterHealRefused : NULL));
    memset(&ctx, 0, sizeof(ctx));
    CeModes_ScriptRestHealGate(&ctx);
    EXPECT(ctx.scriptPtr == (rest ? CeEventScript_RestHealRefused : NULL));
}

TEST("CE: Hard mode: no bag in trainer battles, only Poké Balls in wild ones")
{
    struct CeModesConfig rules = {0};
    bool32 noItems = FALSE;

    PARAMETRIZE { noItems = FALSE; }
    PARAMETRIZE { noItems = TRUE; }

    rules.noBattleItems = noItems;
    rules.forceSetStyle = TRUE;
    CeTest_StartRun(&rules);

    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    EXPECT_EQ(CeModes_NoBagInTrainerBattles(), noItems);
    EXPECT_EQ(IsAllowedToUseBag(), !noItems);
    EXPECT_EQ(CeModes_BlocksBattleItem(ITEM_POTION), noItems);

    gBattleTypeFlags = 0;
    EXPECT(!CeModes_NoBagInTrainerBattles());
    EXPECT_EQ(CeModes_BlocksBattleItem(ITEM_POTION), noItems);
    EXPECT(!CeModes_BlocksBattleItem(ITEM_POKE_BALL));

    // Battle Frontier and other exempt battles keep their own rules.
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER;
    EXPECT(!CeModes_BlocksBattleItem(ITEM_POTION));
    gBattleTypeFlags = 0;
}

TEST("CE: Hard mode: battle style is locked to Set")
{
    struct CeModesConfig rules = {0};
    bool32 forceSet = FALSE;

    PARAMETRIZE { forceSet = FALSE; }
    PARAMETRIZE { forceSet = TRUE; }

    rules.forceSetStyle = forceSet;
    rules.noBattleItems = TRUE;
    CeTest_StartRun(&rules);
    EXPECT_EQ(CeModes_ForcesSetStyle(), forceSet);
    EXPECT_EQ(CeOptions_Effective(CE_VOPT_BATTLE_STYLE, OPTIONS_BATTLE_STYLE_SHIFT), forceSet ? OPTIONS_BATTLE_STYLE_SET : OPTIONS_BATTLE_STYLE_SHIFT);
}

TEST("CE: Nuzlocke forced nicknames follow the rules")
{
    struct CeModesConfig rules = { .nuzlocke = TRUE };
    bool32 force = FALSE;

    PARAMETRIZE { force = FALSE; }
    PARAMETRIZE { force = TRUE; }

    rules.forceNicknames = force;
    CeTest_StartRun(&rules);
    EXPECT_EQ(CeModes_ForcesNickname(), force);
}
