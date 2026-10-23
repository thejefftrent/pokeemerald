#ifndef GUARD_CE_TEAM_TABLE_H
#define GUARD_CE_TEAM_TABLE_H

#include "data.h"
#include "ce/team.h"

// One Pokémon of the static team (see ce/team.h). `mon` is built like a trainer's party
// Pokémon (GenerateMonFromTrainerMon), with the player as its trainer.
struct CeTeamMon
{
    struct TrainerMon mon; // mon.species SPECIES_NONE ends the list
    u8 box;                // 0-13, or CE_TEAM_IN_PARTY (party Pokémon come first, in party order)
    u8 slot;               // 0-29 in the box
    u8 badges;             // badges needed before it arrives (0 = at New Game; always 0 in the party)
    u8 padding;
};

// Written by the website (CE_PATCHABLE, src/ce/team_data.c); all zero in the base ROM.
extern const struct CeTeamMon gCeTeamMons[CE_TEAM_MAX_MONS];

#endif // GUARD_CE_TEAM_TABLE_H
