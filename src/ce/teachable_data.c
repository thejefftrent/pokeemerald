#include "global.h"
#include "item.h"
#include "ce/teachable.h"
#include "constants/moves.h"

// Data only: the website rewrites these, so code reads them through `extern` in other files.
// The per-species bits live in gSpeciesInfo (src/data/pokemon/species_info.h).

#include "../data/pokemon/teachable_learnsets.h"

STATIC_ASSERT(CE_TEACHABLE_GEN_MACHINES == NUM_ALL_MACHINES, CeTeachableGeneratedForThisTmList);

const u16 gCeTutorMoves[CE_TUTOR_SLOTS] CE_PATCHABLE = CE_TEACHABLE_TUTOR_MOVES;
