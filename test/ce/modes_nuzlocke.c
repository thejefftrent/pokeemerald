#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "malloc.h"
#include "pokedex.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "test/test.h"
#include "ce/modes.h"
#include "ce/test.h"
#include "constants/battle.h"
#include "constants/region_map_sections.h"

// Nuzlocke (ce/modes.h). The battle test runner plays recorded battles, which the rules exempt
// like real recorded battles, so these call the end-of-battle hooks the way battle_main.c does.

static void StandIn(u32 mapsec)
{
    ZeroPlayerPartyMons(); // gParties isn't part of the save the runner clears
    FlagSet(FLAG_SYS_POKEDEX_GET); // the rules start with the Pokédex
    gMapHeader.regionMapSectionId = mapsec;
}

static void GivePartyMon(u32 slot, enum Species species, u32 hp, enum Item item)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][slot];

    CreateMon(mon, species, 20, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    CalculatePlayerPartyCount();
}

static void SetCaught(enum Species species)
{
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT);
}

// A wild battle against `foe` ends with `outcome`; playerSentOut = party slots that fought.
static void EndWildBattle(enum Species foe, bool32 shiny, u32 outcome, u32 playerSentOut)
{
    u32 i;

    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gBattleTypeFlags = 0;
    CreateMon(&gParties[B_TRAINER_OPPONENT_A][0], foe, 5, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_IS_SHINY, &shiny);
    gBattleOutcome = outcome;
    gBattleResults.caughtMonSpecies = outcome == B_OUTCOME_CAUGHT ? foe : SPECIES_NONE;
    for (i = 0; i < PARTY_SIZE; i++)
        gBattleStruct->partyState[B_TRAINER_PLAYER][i].sentOut = (playerSentOut >> i) & 1;
    CeModes_OnBattleEnd();
    CeModes_AfterBattle();
    FREE_AND_SET_NULL(gBattleStruct);
    ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][0]);
}

TEST("CE: Nuzlocke: only the first wild encounter in an area can be caught")
{
    static const struct CeModesConfig rules = { .nuzlocke = TRUE };
    u32 area;

    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    area = CeModes_CurrentArea();
    EXPECT_EQ(area, MAPSEC_ROUTE_101);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, area, CE_BATTLE_WILD), CE_CATCH_OK);

    // Running away still uses up the area.
    EndWildBattle(SPECIES_ZIGZAGOON, FALSE, B_OUTCOME_RAN, 0);
    EXPECT(CeModes_IsAreaUsed(area));
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, area, CE_BATTLE_WILD), CE_CATCH_AREA_USED);
    // Other areas, static encounters and trainers are unaffected.
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, MAPSEC_ROUTE_102, CE_BATTLE_WILD), CE_CATCH_OK);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_KYOGRE, FALSE, area, CE_BATTLE_SCRIPTED), CE_CATCH_OK);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, area, CE_BATTLE_TRAINER), CE_CATCH_OK);
}

TEST("CE: Nuzlocke: no area rule before the Pokédex or without the rule")
{
    static const struct CeModesConfig nuzlocke = { .nuzlocke = TRUE };
    static const struct CeModesConfig none = {0};

    CeTest_StartRun(&nuzlocke);
    gMapHeader.regionMapSectionId = MAPSEC_ROUTE_101;
    EXPECT_EQ(CeModes_CurrentArea(), CE_AREA_NONE);
    EndWildBattle(SPECIES_ZIGZAGOON, FALSE, B_OUTCOME_RAN, 0);
    EXPECT(!CeModes_IsAreaUsed(MAPSEC_ROUTE_101));

    CeTest_StartRun(&none);
    StandIn(MAPSEC_ROUTE_101);
    EndWildBattle(SPECIES_ZIGZAGOON, FALSE, B_OUTCOME_RAN, 0);
    EXPECT(!CeModes_IsAreaUsed(MAPSEC_ROUTE_101));
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_OK);
}

TEST("CE: Nuzlocke dupes clause: an owned family is not the area's encounter")
{
    struct CeModesConfig rules = { .nuzlocke = TRUE };
    bool32 dupes = FALSE;

    PARAMETRIZE { dupes = FALSE; }
    PARAMETRIZE { dupes = TRUE; }

    rules.dupesClause = dupes;
    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    SetCaught(SPECIES_ZIGZAGOON);
    EXPECT_EQ(CeModes_FamilyOwned(SPECIES_ZIGZAGOON), TRUE);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), dupes ? CE_CATCH_DUPE : CE_CATCH_OK);
    EXPECT_EQ(CeModes_CountsAsEncounter(SPECIES_ZIGZAGOON, FALSE), !dupes);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_POOCHYENA, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_OK);

    // Meeting the dupe leaves the area open under the clause.
    EndWildBattle(SPECIES_ZIGZAGOON, FALSE, B_OUTCOME_RAN, 0);
    EXPECT_EQ(CeModes_IsAreaUsed(MAPSEC_ROUTE_101), !dupes);
}

TEST("CE: Nuzlocke shiny clause: shinies can always be caught and don't use the area")
{
    struct CeModesConfig rules = { .nuzlocke = TRUE, .dupesClause = TRUE };
    bool32 shinyClause = FALSE;

    PARAMETRIZE { shinyClause = FALSE; }
    PARAMETRIZE { shinyClause = TRUE; }

    rules.shinyClause = shinyClause;
    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    CeModes_UseArea(MAPSEC_ROUTE_101);
    SetCaught(SPECIES_ZIGZAGOON);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, TRUE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), shinyClause ? CE_CATCH_OK : CE_CATCH_AREA_USED);
    EXPECT_EQ(CeModes_CatchRule(SPECIES_ZIGZAGOON, FALSE, MAPSEC_ROUTE_101, CE_BATTLE_WILD), CE_CATCH_AREA_USED);

    // A fresh area: catching a shiny there doesn't use it up under the clause.
    gMapHeader.regionMapSectionId = MAPSEC_ROUTE_102;
    EndWildBattle(SPECIES_POOCHYENA, TRUE, B_OUTCOME_CAUGHT, 0);
    EXPECT_EQ(CeModes_IsAreaUsed(MAPSEC_ROUTE_102), !shinyClause);
}

TEST("CE: Nuzlocke: Pokémon that faint in battle are dead")
{
    static const struct CeModesConfig rules = { .nuzlocke = TRUE };
    struct Pokemon *fainted = &gParties[B_TRAINER_PLAYER][0];
    struct Pokemon *benched = &gParties[B_TRAINER_PLAYER][2];

    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    GivePartyMon(0, SPECIES_WOBBUFFET, 0, ITEM_NONE);
    GivePartyMon(1, SPECIES_WYNAUT, 30, ITEM_NONE);
    GivePartyMon(2, SPECIES_ZIGZAGOON, 0, ITEM_NONE); // fainted before, didn't fight
    EndWildBattle(SPECIES_POOCHYENA, FALSE, B_OUTCOME_WON, (1 << 0) | (1 << 1));
    EXPECT(CeModes_IsDead(fainted));
    EXPECT(!CeModes_IsDead(&gParties[B_TRAINER_PLAYER][1]));
    EXPECT(!CeModes_IsDead(benched));
    EXPECT_EQ(gSaveBlock2Ptr->pokedex.ceModes.deaths, 1);

    // The dead can't be healed, revived or given items.
    HealPlayerParty();
    EXPECT_EQ(GetMonData(fainted, MON_DATA_HP), 0);
    EXPECT_EQ(GetMonData(benched, MON_DATA_HP), GetMonData(benched, MON_DATA_MAX_HP));
    EXPECT_EQ(PokemonUseItemEffects(fainted, ITEM_REVIVE, 0, 0, FALSE), TRUE);
    EXPECT_EQ(GetMonData(fainted, MON_DATA_HP), 0);
}

TEST("CE: Nuzlocke: no deaths without the rule")
{
    static const struct CeModesConfig rules = { .noCenterHeal = TRUE };

    CeTest_StartRun(&rules);
    GivePartyMon(0, SPECIES_WOBBUFFET, 0, ITEM_NONE);
    GivePartyMon(1, SPECIES_WYNAUT, 30, ITEM_NONE);
    EndWildBattle(SPECIES_POOCHYENA, FALSE, B_OUTCOME_WON, 1 << 0);
    EXPECT(!CeModes_IsDead(&gParties[B_TRAINER_PLAYER][0]));
}

TEST("CE: Nuzlocke: the dead are released after battle, items to the bag")
{
    struct CeModesConfig rules = { .nuzlocke = TRUE };
    bool32 release = FALSE;

    PARAMETRIZE { release = FALSE; }
    PARAMETRIZE { release = TRUE; }

    rules.releaseDead = release;
    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    GivePartyMon(0, SPECIES_WOBBUFFET, 0, ITEM_ORAN_BERRY);
    GivePartyMon(1, SPECIES_WYNAUT, 30, ITEM_NONE);
    EndWildBattle(SPECIES_POOCHYENA, FALSE, B_OUTCOME_WON, (1 << 0) | (1 << 1));
    if (release)
    {
        EXPECT_EQ(CalculatePlayerPartyCount(), 1);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_WYNAUT);
        EXPECT(CheckBagHasItem(ITEM_ORAN_BERRY, 1));
    }
    else
    {
        EXPECT_EQ(CalculatePlayerPartyCount(), 2);
        EXPECT(CeModes_IsDead(&gParties[B_TRAINER_PLAYER][0]));
        EXPECT(!CheckBagHasItem(ITEM_ORAN_BERRY, 1));
    }
}

TEST("CE: Nuzlocke: the last Pokémon isn't released while none is alive")
{
    static const struct CeModesConfig rules = { .nuzlocke = TRUE, .releaseDead = TRUE };

    CeTest_StartRun(&rules);
    StandIn(MAPSEC_ROUTE_101);
    GivePartyMon(0, SPECIES_WOBBUFFET, 0, ITEM_NONE);
    EndWildBattle(SPECIES_POOCHYENA, FALSE, B_OUTCOME_LOST, 1 << 0);
    EXPECT(CeModes_IsDead(&gParties[B_TRAINER_PLAYER][0]));
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
}
