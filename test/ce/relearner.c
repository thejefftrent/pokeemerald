#include "global.h"
#include "malloc.h"
#include "move_relearner.h"
#include "party_menu.h"
#include "pokemon.h"
#include "test/test.h"
#include "ce/relearner.h"
#include "ce/teachable.h"
#include "constants/moves.h"

// The move relearner's egg and tutor lists (ce/relearner.h). The lists are read through
// CeRelearner_GetListMoves, the relearner's own state table, so they are what the screen shows.

// A Pokémon that knows only `move`.
static void CreateMonKnowing(struct Pokemon *mon, u32 species, u32 level, u16 move)
{
    u16 none = MOVE_NONE;
    u32 i;

    CreateMon(mon, species, level, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(mon, MON_DATA_MOVE1, &move);
    for (i = 1; i < MAX_MON_MOVES; i++)
        SetMonData(mon, MON_DATA_MOVE1 + i, &none);
}

static u32 CountInList(const u16 *moves, u32 count, u32 move)
{
    u32 i, n = 0;

    for (i = 0; i < count; i++)
    {
        if (moves[i] == move)
            n++;
    }
    return n;
}

static void ExpectNoDuplicates(const u16 *moves, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++)
        EXPECT_EQ(CountInList(moves, count, moves[i]), 1);
}

TEST("CE: the egg move relearner lists every stage's egg moves, the first stage's first")
{
    struct Pokemon mon;
    u16 *moves = Alloc(CE_RELEARNER_MAX_MOVES * sizeof(u16));
    const u16 *azurill = GetSpeciesEggMoves(SPECIES_AZURILL);
    const u16 *marill = GetSpeciesEggMoves(SPECIES_MARILL);
    u32 count, i, next = 0;

    CreateMonKnowing(&mon, SPECIES_AZUMARILL, 50, MOVE_TACKLE);
    count = CeRelearner_GetListMoves(&mon.box, MOVE_RELEARNER_EGG_MOVES, moves);

    // Marill's own egg moves (upstream listed only Azurill's) and Azurill's.
    EXPECT_EQ(CountInList(moves, count, MOVE_BELLY_DRUM), 1);
    EXPECT_EQ(CountInList(moves, count, MOVE_AQUA_JET), 1);
    EXPECT_EQ(CountInList(moves, count, MOVE_SING), 1);
    // Azurill's list comes first, in its order; then Marill's moves Azurill doesn't have.
    for (i = 0; azurill[i] != MOVE_UNAVAILABLE; i++)
        EXPECT_EQ(moves[next++], azurill[i]);
    for (i = 0; marill[i] != MOVE_UNAVAILABLE; i++)
    {
        if (CountInList(azurill, next, marill[i]) == 0)
            EXPECT_EQ(moves[next++], marill[i]);
    }
    EXPECT_EQ(count, next);
    ExpectNoDuplicates(moves, count);
    Free(moves);
}

TEST("CE: the egg move relearner leaves out moves the Pokémon knows")
{
    struct Pokemon mon;
    u16 *moves = Alloc(CE_RELEARNER_MAX_MOVES * sizeof(u16));
    const u16 *bulbasaur = GetSpeciesEggMoves(SPECIES_BULBASAUR);
    u32 count, i;

    CreateMonKnowing(&mon, SPECIES_VENUSAUR, 50, MOVE_CHARM);
    count = CeRelearner_GetListMoves(&mon.box, MOVE_RELEARNER_EGG_MOVES, moves);

    EXPECT_EQ(CountInList(moves, count, MOVE_CHARM), 0);
    for (i = 0; bulbasaur[i] != MOVE_UNAVAILABLE; i++)
        EXPECT_EQ(CountInList(moves, count, bulbasaur[i]), bulbasaur[i] != MOVE_CHARM);
    // Endure is a Bulbasaur egg move and a tutor move: once here.
    EXPECT_EQ(CountInList(moves, count, MOVE_ENDURE), 1);
    // A TM move that isn't an egg move stays out.
    EXPECT_EQ(CountInList(moves, count, MOVE_SOLAR_BEAM), 0);
    ExpectNoDuplicates(moves, count);
    Free(moves);
}

TEST("CE: a Pokémon has egg moves to relearn exactly when the egg list isn't empty")
{
    struct Pokemon mon;
    u16 *moves = Alloc(CE_RELEARNER_MAX_MOVES * sizeof(u16));
    u32 species = SPECIES_NONE, count;
    u16 known = MOVE_TACKLE;

    PARAMETRIZE { species = SPECIES_BULBASAUR; }
    PARAMETRIZE { species = SPECIES_IVYSAUR; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; }
    PARAMETRIZE { species = SPECIES_AZUMARILL; }
    PARAMETRIZE { species = SPECIES_MEW; }      // no egg moves
    PARAMETRIZE { species = SPECIES_DITTO; }    // none either
    PARAMETRIZE { species = SPECIES_SHEDINJA; } // Nincada's line; it has none of its own
    PARAMETRIZE { species = SPECIES_PICHU; known = MOVE_VOLT_TACKLE; }

    CreateMonKnowing(&mon, species, 50, known);
    count = CeRelearner_GetListMoves(&mon.box, MOVE_RELEARNER_EGG_MOVES, moves);
    EXPECT_EQ(CeRelearner_HasEggMoves(&mon.box), count > 0);
    EXPECT_EQ(HasMoveToRelearn(&mon.box, MOVE_RELEARNER_EGG_MOVES), count > 0);
    EXPECT_LE(count, CE_RELEARNER_MAX_MOVES);
    if (species == SPECIES_MEW || species == SPECIES_DITTO)
        EXPECT_EQ(count, 0);
    else
        EXPECT_GT(count, 0);
    Free(moves);
}

TEST("CE: the tutor move relearner lists every tutor move the species can learn, in slot order")
{
    struct Pokemon mon;
    u16 *moves = Alloc(CE_RELEARNER_MAX_MOVES * sizeof(u16));
    u32 species = SPECIES_NONE, count, i, next = 0;

    PARAMETRIZE { species = SPECIES_MEW; }
    PARAMETRIZE { species = SPECIES_VENUSAUR; }
    PARAMETRIZE { species = SPECIES_MAGIKARP; } // learns none

    CreateMonKnowing(&mon, species, 50, MOVE_TACKLE);
    count = CeRelearner_GetListMoves(&mon.box, MOVE_RELEARNER_TUTOR_MOVES, moves);
    for (i = 0; i < CE_TUTOR_SLOTS; i++)
    {
        u32 move = gCeTutorMoves[i];
        if (move != MOVE_NONE && move != MOVE_TACKLE && CanLearnTeachableMove(species, move))
            EXPECT_EQ(moves[next++], move);
    }
    EXPECT_EQ(count, next);
    EXPECT_LE(count, CE_TUTOR_SLOTS);
    EXPECT_EQ(HasMoveToRelearn(&mon.box, MOVE_RELEARNER_TUTOR_MOVES), count > 0);
    if (species == SPECIES_MAGIKARP)
        EXPECT_EQ(count, 0);
    else
        EXPECT_GT(count, 0);
    Free(moves);
}
