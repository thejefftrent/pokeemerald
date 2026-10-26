#ifndef GUARD_CE_RELEARNER_H
#define GUARD_CE_RELEARNER_H

#include "constants/move_relearner.h"

// The move relearner's egg and tutor lists (docs/learnsets.md).
//
// One-line hooks in upstream code:
//   src/move_relearner.c             the list buffer holds CE_RELEARNER_MAX_MOVES (heap struct);
//                                    GetRelearnerEggMoves, HasRelearnerEggMoves call ours;
//                                    CeRelearner_GetListMoves (end of file)

// Room in the relearner's list (heap, while the relearner screen is open). MAX_RELEARNER_MOVES
// (60) stays the most entries the website lets a level-up or egg list hold; a relearner list can
// be longer: all CE_TUTOR_SLOTS tutor moves, or the egg lists of a whole evolution line.
#define CE_RELEARNER_MAX_MOVES  200
// The longest evolution line the egg list walks back (a guard: real lines have three stages).
#define CE_RELEARNER_MAX_STAGES 8

struct BoxPokemon;

// The egg moves the relearner offers: every egg move of the Pokémon's evolution line, first
// stage first (Bulbasaur's, then Ivysaur's, then Venusaur's own), each once, without the moves
// it knows. The line follows GetSpeciesPreEvolution back from the species, stopping at a species
// it already passed (an evolution cycle made on the website) or after CE_RELEARNER_MAX_STAGES.
// `moves` holds CE_RELEARNER_MAX_MOVES; longer lists are cut there. Returns how many.
u32 CeRelearner_GetEggMoves(struct BoxPokemon *mon, u16 *moves);
// Whether CeRelearner_GetEggMoves would list anything.
bool32 CeRelearner_HasEggMoves(struct BoxPokemon *mon);
// The list the relearner shows for `state` (move_relearner.c's table, so tests and the self-test
// read exactly what the screen does). `moves` holds CE_RELEARNER_MAX_MOVES.
u32 CeRelearner_GetListMoves(struct BoxPokemon *mon, enum MoveRelearnerStates state, u16 *moves);

#endif // GUARD_CE_RELEARNER_H
