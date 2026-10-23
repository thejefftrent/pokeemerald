#include "global.h"
#include "pokemon.h"
#include "constants/species.h"

// Data only: the website rewrites this table, so code reads it through `extern` in other files.

// Innate third type of each species (enum Type); TYPE_MYSTERY = none.
CE_PATCHABLE const u8 gCeSpeciesThirdType[NUM_SPECIES] = { [0 ... NUM_SPECIES - 1] = TYPE_MYSTERY };
