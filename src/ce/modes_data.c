#include "global.h"
#include "ce/modes.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"

// Data only: no code in this file may read these tables, so the compiler never sees an
// initializer it could fold. Everything else sees them through `extern`.

// Nuzlocke area of each map section: CE_AREA_OWN, CE_AREA_FREE, or area + 1 (merged sections).
CE_PATCHABLE const u8 gCeAreaOfMapsec[MAPSEC_COUNT] = {0};

// Root species of each species' evolution family (SPECIES_NONE = the species itself). The
// website computes it from the final evolutions whenever a mode compares families.
CE_PATCHABLE const u16 gCeFamilyRoot[NUM_SPECIES] = {0};
