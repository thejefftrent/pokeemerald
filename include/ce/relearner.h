#ifndef GUARD_CE_RELEARNER_H
#define GUARD_CE_RELEARNER_H

#include "constants/move_relearner.h"

// The move relearner's egg and tutor lists (docs/qol-extras.md, docs/learnsets.md).
//
//   CE_QOL_RELEARNER_EGG_TUTOR  The Fallarbor Town Move Relearner, and the Move Relearner key
//                               item, first ask for level-up, egg or tutor moves. Fallarbor still
//                               takes a Heart Scale per move; the key item stays free.
//
// They run upstream's relearner in script mode with its own states, MOVE_RELEARNER_EGG_MOVES and
// MOVE_RELEARNER_TUTOR_MOVES (script mode ignores the states' isActive, so the summary screen's
// relearner keeps level-up moves only). The scripts are in data/scripts/ce/relearner.inc.
//
// One-line hooks in upstream code:
//   src/move_relearner.c             the list buffer holds CE_RELEARNER_MAX_MOVES (heap struct);
//                                    GetRelearnerEggMoves, HasRelearnerEggMoves call ours;
//                                    CeRelearner_GetListMoves (end of file)
//   src/pokemon_summary_screen.c     UpdateMoveRelearnerState: a script relearner keeps its list
//                                    through the forget-a-move screen
//   data/maps/FallarborTown_MoveRelearnersHouse/scripts.inc  ChooseMon: call
//                                    CE_EventScript_RelearnerFallarborList
//   data/event_scripts.s             #include "data/scripts/ce/relearner.inc"

// Room in the relearner's list (heap, while the relearner screen is open). MAX_RELEARNER_MOVES
// (60) stays the most entries the website lets a level-up or egg list hold; a relearner list can
// be longer: all CE_TUTOR_SLOTS tutor moves, or the egg lists of a whole evolution line.
#define CE_RELEARNER_MAX_MOVES  200
// The longest evolution line the egg list walks back (a guard: real lines have three stages).
#define CE_RELEARNER_MAX_STAGES 8

struct BoxPokemon;
struct ScriptContext;

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

// Whether the Fallarbor relearner and the key item ask which list (CE_QOL_RELEARNER_EGG_TUTOR).
bool32 CeRelearner_OffersEggTutor(void);

// Script native (callnative, requests_effects=1) for data/scripts/ce/relearner.inc: sets the
// level-up list, as Fallarbor always did, and VAR_RESULT = CeRelearner_OffersEggTutor().
void CeRelearner_ScriptStartList(struct ScriptContext *ctx);

#endif // GUARD_CE_RELEARNER_H
