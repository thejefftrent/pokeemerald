#include "global.h"
#include "pokedex.h"
#include "pokedex_common.h"
#include "pokemon.h"
#include "strings.h"
#include "task.h"
#include "ce/pokedex.h"
#include "constants/pokedex.h"

STATIC_ASSERT(ARRAY_COUNT(((struct EvoScreenData *)0)->targetSpecies) == CE_DEX_EVO_ROWS, CeDexEvoRowsTargets);
STATIC_ASSERT(ARRAY_COUNT(((struct EvoScreenData *)0)->arrowSpriteDist) == CE_DEX_EVO_ROWS, CeDexEvoRowsArrows);
STATIC_ASSERT(ARRAY_COUNT(((struct EvoScreenData *)0)->seen) == CE_DEX_EVO_ROWS, CeDexEvoRowsSeen);
STATIC_ASSERT(5 + CE_DEX_FORM_ICONS <= NUM_TASK_DATA, CeDexFormIconsFitTask);

bool32 CeDex_IsSeen(enum Species species)
{
    enum NationalDexOrder dexNum;

    // Edited evolution data can name any id: check it here, before SanitizeSpeciesId's asserts.
    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return FALSE;
    // GetSetPokedexFlag(NATIONAL_DEX_NONE) would read before the flags.
    dexNum = SpeciesToNationalPokedexNum(species);
    return dexNum != NATIONAL_DEX_NONE && dexNum <= NATIONAL_DEX_COUNT && GetSetPokedexFlag(dexNum, FLAG_GET_SEEN);
}

enum Species CeDex_SeenOrUnknown(enum Species species)
{
    return CeDex_IsSeen(species) ? species : SPECIES_NONE;
}

const u8 *CeDex_SeenName(enum Species species)
{
    return CeDex_IsSeen(species) ? GetSpeciesName(species) : gText_ThreeQuestionMarks;
}
