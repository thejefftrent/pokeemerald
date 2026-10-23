#include "global.h"
#include "event_data.h"
#include "field_move.h"
#include "party_menu.h"
#include "pokemon.h"
#include "test/overworld_script.h"
#include "test/test.h"
#include "ce/field_moves.h"
#include "ce/test.h"
#include "constants/field_move.h"

// Field moves without HMs (ce/field_moves.h). The field effects need a loaded map; these test
// the gates the scripts and the party menu go through. The script DSL takes numbers, not enums.
#define SCRIPT_CUT          0
#define SCRIPT_SOFT_BOILED 12

ASSUMPTIONS
{
    ASSUME(FIELD_MOVE_CUT == SCRIPT_CUT);
    ASSUME(FIELD_MOVE_SOFT_BOILED == SCRIPT_SOFT_BOILED);
}

// Party: an Egg, then a Zigzagoon that knows no HM move.
static void GiveParty(void)
{
    bool32 isEgg = TRUE;

    ZeroPlayerPartyMons(); // gParties isn't part of the save the runner clears
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_WURMPLE, 5, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG, &isEgg);
    CreateMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PLAYER_ID);
    CalculatePlayerPartyCount();
}

TEST("CE: Field moves without HMs: the first Pokémon that isn't an Egg uses the move")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_ON; }

    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, mode);
    GiveParty();
    ASSUME(!MonKnowsMove(&gParties[B_TRAINER_PLAYER][1], MOVE_CUT));
    FlagSet(FLAG_BADGE01_GET);
    RUN_OVERWORLD_SCRIPT(
        checkfieldmove SCRIPT_CUT, TRUE;
    );
    if (mode == CE_QOL_MODE_OFF)
    {
        EXPECT_EQ(gSpecialVar_Result, PARTY_SIZE);
    }
    else
    {
        EXPECT_EQ(gSpecialVar_Result, 1);
        EXPECT_EQ(gSpecialVar_0x8004, SPECIES_ZIGZAGOON);
    }
}

TEST("CE: Field moves without HMs still need the move's badge")
{
    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, CE_QOL_MODE_ON);
    GiveParty();
    RUN_OVERWORLD_SCRIPT(
        checkfieldmove SCRIPT_CUT, FALSE;
    );
    EXPECT_EQ(gSpecialVar_Result, PARTY_SIZE);
}

TEST("CE: Field moves without HMs: a Pokémon that knows the move still uses it")
{
    enum Move cut = MOVE_CUT;

    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, CE_QOL_MODE_ON);
    GiveParty();
    CreateMon(&gParties[B_TRAINER_PLAYER][2], SPECIES_LINOONE, 30, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_MOVE1, &cut);
    CalculatePlayerPartyCount();
    FlagSet(FLAG_BADGE01_GET);
    RUN_OVERWORLD_SCRIPT(
        checkfieldmove SCRIPT_CUT, TRUE;
    );
    EXPECT_EQ(gSpecialVar_Result, 2);
    EXPECT_EQ(gSpecialVar_0x8004, SPECIES_LINOONE);
}

TEST("CE: Field moves without HMs leave other field moves alone")
{
    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, CE_QOL_MODE_ON);
    GiveParty();
    RUN_OVERWORLD_SCRIPT(
        checkfieldmove SCRIPT_SOFT_BOILED, FALSE;
    );
    EXPECT_EQ(gSpecialVar_Result, PARTY_SIZE);
}

TEST("CE: Surf without a Pokémon that knows it, once the Balance Badge is owned")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    bool32 badge = FALSE;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; badge = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  badge = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  badge = TRUE; }

    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, mode);
    GiveParty();
    if (badge)
        FlagSet(FLAG_BADGE05_GET);
    else
        FlagClear(FLAG_BADGE05_GET);
    EXPECT_EQ(CeFieldMoves_CanSurfWithoutMove(), mode == CE_QOL_MODE_ON && badge);
}
