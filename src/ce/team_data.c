#include "global.h"
#include "ce/team_table.h"

// Data only: no code in this file may read this table, so the compiler never sees an
// initializer it could fold. Everything else sees it through `extern`.

// The static team (include/ce/team_table.h). All zero: no team.
CE_PATCHABLE const struct CeTeamMon gCeTeamMons[CE_TEAM_MAX_MONS] = {0};
