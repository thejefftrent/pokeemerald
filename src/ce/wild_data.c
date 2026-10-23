#include "global.h"
#include "fieldmap.h"
#include "wild_encounter.h"
#include "ce/wild.h"

// Data only: no code in this file may read gCeWildMonHeaders, so the compiler never sees
// an initializer it could fold. Everything else sees it through `extern`.

// The generated table keeps its name (src/data/wild_encounters.h); readers use the pointer.
#undef gWildMonHeaders
extern const struct WildPokemonHeader gWildMonHeaders[];

// The header array the game reads, terminated by mapGroup == MAP_GROUP(MAP_UNDEFINED).
// The website repoints it at a longer copy in free space to give maps new tables.
CE_PATCHABLE const struct WildPokemonHeader *const gCeWildMonHeaders = gWildMonHeaders;

const struct CeWildTerrainInfo gCeWildTerrainInfo =
{
    .metatileIdMask = MAPGRID_METATILE_ID_MASK,
    .primaryMetatiles = NUM_METATILES_IN_PRIMARY,
    .behaviorMask = METATILE_ATTR_BEHAVIOR_MASK,
};
