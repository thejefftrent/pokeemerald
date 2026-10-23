#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "test/battle.h"
#include "ce/modes.h"
#include "ce/test.h"

// Single-Pokémon mode with a party limit of 1 (ce/modes.h): a second Pokémon can never join, so
// double battles that are started by talking to the trainer (Tate & Liza, twins, rematches) are
// fought 1-vs-2, as with expansion's OW_DOUBLE_APPROACH_WITH_ONE_MON. Double trainers still don't
// walk up to a party of one, so the optional ones stay optional.

void HasEnoughMonsForDoubleBattle(void); // a script special, declared only in data/specials.inc

static void GiveOnlyMon(void)
{
    ZeroPlayerPartyMons(); // gParties isn't part of the save the runner clears
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PLAYER_ID);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][0]);
    CalculatePlayerPartyCount();
}

static u32 SpecialResult(void)
{
    gSpecialVar_Result = 0xFFFF;
    HasEnoughMonsForDoubleBattle();
    return gSpecialVar_Result;
}

// test/battle.h replaces PARAMETRIZE, so the three rule sets run one after another.
TEST("CE: Single-Pokémon: a party limit of 1 lets double battles start with one Pokémon")
{
    static const struct CeModesConfig none = {0};
    static const struct CeModesConfig limit1 = { .partyLimit = 1 };
    static const struct CeModesConfig limit2 = { .partyLimit = 2 };

    // Vanilla, and any limit that leaves room for a second Pokémon: "we can't battle that way".
    GiveOnlyMon();
    CeTest_StartRun(&none);
    EXPECT(!HasEnoughMonsForDoubleBattle2());
    EXPECT_EQ(SpecialResult(), PLAYER_HAS_ONE_MON);
    CeTest_StartRun(&limit2);
    EXPECT(!HasEnoughMonsForDoubleBattle2());
    EXPECT_EQ(SpecialResult(), PLAYER_HAS_ONE_MON);

    // A limit of 1: BattleSetup_ConfigureTrainerBattle and the double-battle scripts go ahead.
    CeTest_StartRun(&limit1);
    EXPECT(HasEnoughMonsForDoubleBattle2());
    EXPECT_EQ(SpecialResult(), PLAYER_HAS_TWO_USABLE_MONS);
    // Trainer sight (trainer_see.c) is unchanged: double trainers don't approach a party of one.
    EXPECT_EQ(GetMonsStateToDoubles_2(), PLAYER_HAS_ONE_MON);
}

// What the battle then looks like. The runner wants two PLAYER Pokémon in a double battle, so the
// second is an Egg, which can't fight either: the right slot is absent from the start
// (TryDoEventsBeforeFirstTurn), as for a party of one, and both foes attack the one Pokémon.
DOUBLE_BATTLE_TEST("CE: Single-Pokémon: a double battle with one Pokémon that can fight plays 1-vs-2")
{
    bool32 isEgg = TRUE;

    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        PLAYER(SPECIES_WYNAUT);
        OPPONENT(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
        SetMonData(&PLAYER_PARTY[1], MON_DATA_IS_EGG, &isEgg);
    } WHEN {
        TURN { MOVE(playerLeft, MOVE_SCRATCH, target: opponentRight); MOVE(opponentLeft, MOVE_SCRATCH, target: playerLeft); MOVE(opponentRight, MOVE_SCRATCH, target: playerLeft); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, playerLeft);
        HP_BAR(opponentRight);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponentLeft);
        HP_BAR(playerLeft);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, opponentRight);
        HP_BAR(playerLeft);
    } THEN {
        EXPECT(gAbsentBattlerFlags & (1u << B_POSITION_PLAYER_RIGHT));
    }
}
