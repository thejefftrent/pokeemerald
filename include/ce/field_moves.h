#ifndef GUARD_CE_FIELD_MOVES_H
#define GUARD_CE_FIELD_MOVES_H

#include "ce/qol.h"
#include "constants/field_move.h"

// Field moves without HMs, and flying from anywhere. One-line hooks in upstream code:
//   src/scrcmd.c              ScrCmd_checkfieldmove          CE_QOL_FIELD_MOVES_WITHOUT_HMS
//   src/field_player_avatar.c PartyHasMonWithSurf            CE_QOL_FIELD_MOVES_WITHOUT_HMS
//   src/party_menu.c          SetPartyMonFieldSelectionActions CE_QOL_FIELD_MOVES_WITHOUT_HMS
//   src/party_menu.c          SetUpFieldMove_Fly             CE_QOL_FLY_FROM_ANYWHERE
//   src/pokenav_region_map.c  HandleRegionMapInput, UpdateRegionMapHelpBarText  CE_QOL_FLY_FROM_ANYWHERE
//   src/field_region_map.c    FieldUpdateRegionMap, PrintTitleWindowText        CE_QOL_FLY_FROM_ANYWHERE
//
// Field moves without HMs: Cut, Flash, Rock Smash, Strength, Surf, Fly, Dive and Waterfall
// work once the player has the move's badge (gFieldMoveInfo, as in vanilla), even if no party
// Pokémon knows the move. The scripts and field effects are vanilla's: when nobody knows the
// move, the first Pokémon that isn't an Egg (or, from the party menu, the chosen one) uses it.
// Every gate is read when used, so switching the feature in the options menu acts at once.

// ScrCmd_checkfieldmove, after vanilla found no Pokémon that knows the move: with the feature
// on and the badge owned, the first party Pokémon that isn't an Egg uses it (VAR_RESULT is its
// slot, VAR_0x8004 its species, as vanilla sets them).
void CeFieldMoves_CheckFieldMove(enum FieldMove fieldMove);

// PartyHasMonWithSurf: TRUE when the player may surf without a Pokémon that knows Surf.
bool32 CeFieldMoves_CanSurfWithoutMove(void);

// SetPartyMonFieldSelectionActions, after the Pokémon's own field moves: the HM field moves it
// doesn't know that can be used right here (facing a tree, rock, boulder, water or waterfall,
// on a dive spot, in a dark cave, or where Fly works), while the action list has room for the
// entries that follow (Switch, Item, Cancel). Choosing one runs vanilla's SetUpFieldMove.
void CeFieldMoves_AddPartyMenuActions(u8 *actions, u8 *numActions, u32 maxActions, u32 fieldMoveAction);

// SetUpFieldMove_Fly: Fly works here. Vanilla: outdoors (towns, cities, routes). With
// CE_QOL_FLY_FROM_ANYWHERE, also wherever an Escape Rope works (caves and dungeons).
bool32 CeFly_AllowedHere(void);

// The region maps' R button (Pokénav map, Town Map): fly to the city under the cursor. Vanilla:
// OW_FLAG_POKE_RIDER (unused in Emerald) and outdoors. With CE_QOL_FLY_FROM_ANYWHERE, once the
// player has Fly's badge, wherever CeFly_AllowedHere and no partner NPC forbids leaving.
bool32 CeFly_CanFlyFromMap(void);

#endif // GUARD_CE_FIELD_MOVES_H
