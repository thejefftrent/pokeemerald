#include "global.h"
#include "event_data.h"
#include "item.h"
#include "move_relearner.h"
#include "party_menu.h"
#include "pokemon.h"
#include "script.h"
#include "ce/qol.h"
#include "ce/relearner.h"
#include "ce/teachable.h"
#include "constants/moves.h"
#include "constants/species.h"

// The egg and tutor relearner lists (ce/relearner.h).

// The list buffer must hold upstream's own lists, every tutor slot, and the Cancel row after the
// last move (numMenuChoices is a u8).
STATIC_ASSERT(CE_RELEARNER_MAX_MOVES >= MAX_RELEARNER_MOVES, CeRelearnerHoldsUpstreamLists);
STATIC_ASSERT(CE_RELEARNER_MAX_MOVES >= CE_TUTOR_SLOTS, CeRelearnerHoldsEveryTutorSlot);
STATIC_ASSERT(CE_RELEARNER_MAX_MOVES >= NUM_ALL_MACHINES, CeRelearnerHoldsEveryMachine);
STATIC_ASSERT(CE_RELEARNER_MAX_MOVES + 1 <= 255, CeRelearnerListFitsAMenu);

// The species and its pre-evolutions, the species first. Stops at the first stage, at a species
// already in the line, or after CE_RELEARNER_MAX_STAGES.
static u32 GetEvolutionLine(u32 species, u16 *line)
{
    u32 count = 0, i;

    while (species != SPECIES_NONE && count < CE_RELEARNER_MAX_STAGES)
    {
        for (i = 0; i < count; i++)
        {
            if (line[i] == species)
                return count;
        }
        line[count++] = species;
        species = GetSpeciesPreEvolution(species);
    }
    return count;
}

static bool32 IsInList(const u16 *moves, u32 count, u32 move)
{
    u32 i;

    for (i = 0; i < count; i++)
    {
        if (moves[i] == move)
            return TRUE;
    }
    return FALSE;
}

u32 CeRelearner_GetEggMoves(struct BoxPokemon *mon, u16 *moves)
{
    u16 line[CE_RELEARNER_MAX_STAGES];
    u32 stages = GetEvolutionLine(GetBoxMonData(mon, MON_DATA_SPECIES), line);
    u32 count = 0;

    while (stages-- > 0)
    {
        const u16 *eggMoves = GetSpeciesEggMoves(line[stages]);
        u32 i;

        for (i = 0; eggMoves[i] != MOVE_UNAVAILABLE && count < CE_RELEARNER_MAX_MOVES; i++)
        {
            u32 move = eggMoves[i];

            if (move == MOVE_NONE || BoxMonKnowsMove(mon, move) || IsInList(moves, count, move))
                continue;
            moves[count++] = move;
        }
    }
    return count;
}

// Same answer as CeRelearner_GetEggMoves(...) > 0. The species' own list is checked first, so a
// Pokémon with an egg move left to learn doesn't walk the evolution line (GetSpeciesPreEvolution
// reads every species).
static bool32 SpeciesHasEggMoveToLearn(struct BoxPokemon *mon, u32 species)
{
    const u16 *eggMoves = GetSpeciesEggMoves(species);
    u32 i;

    for (i = 0; eggMoves[i] != MOVE_UNAVAILABLE; i++)
    {
        if (eggMoves[i] != MOVE_NONE && !BoxMonKnowsMove(mon, eggMoves[i]))
            return TRUE;
    }
    return FALSE;
}

bool32 CeRelearner_HasEggMoves(struct BoxPokemon *mon)
{
    u16 line[CE_RELEARNER_MAX_STAGES];
    u32 species = GetBoxMonData(mon, MON_DATA_SPECIES);
    u32 stages, i;

    if (species == SPECIES_NONE)
        return FALSE;
    if (SpeciesHasEggMoveToLearn(mon, species))
        return TRUE;
    stages = GetEvolutionLine(species, line);
    for (i = 1; i < stages; i++)
    {
        if (SpeciesHasEggMoveToLearn(mon, line[i]))
            return TRUE;
    }
    return FALSE;
}

bool32 CeRelearner_OffersEggTutor(void)
{
    return CeQol_IsOn(CE_QOL_RELEARNER_EGG_TUTOR);
}

void CeRelearner_ScriptStartList(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    gMoveRelearnerState = MOVE_RELEARNER_LEVEL_UP_MOVES;
    gSpecialVar_Result = CeRelearner_OffersEggTutor();
}
