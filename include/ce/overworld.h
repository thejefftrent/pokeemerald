#ifndef GUARD_CE_OVERWORLD_H
#define GUARD_CE_OVERWORLD_H

#include "ce/qol.h"

// Overworld QoL. Each feature is switched by one-line hooks in upstream code:
//   src/bike.c                 IsRunningDisallowed            CE_QOL_RUN_INDOORS
//   src/field_player_avatar.c  PlayerNotOnBikeMoving          CE_QOL_RUNNING_SHOES, CE_QOL_AUTO_RUN,
//                                                             CE_QOL_FAST_SURF
//   src/overworld.c            Overworld_IsBikingAllowed      CE_QOL_BIKE_INDOORS
//   src/item_use.c             ItemUseOutOfBattle_Bike        CE_QOL_BIKE_INDOORS (getting off)
//   src/field_control_avatar.c ShouldDisableRandomEncounters  CE_QOL_NO_WILD_ENCOUNTERS
//   src/trainer_see.c          CheckForTrainersWantingBattle  CE_QOL_NO_TRAINER_SIGHT
//   src/overworld.c            LoadMapFromCameraTransition    CE_QOL_HIDE_REPEAT_MAP_POPUP
//   src/overworld.c            ScriptShow/HideItemDescription CE_QOL_ITEM_DESCRIPTIONS
//   src/event_object_movement.c UpdateFollowingPokemon        CE_QOL_FOLLOWERS (spawns nothing while off)
// The gates are read on every step, frame, map change or script command, never cached, so a
// feature flipped from the options menu takes effect on the next step.

// PlayerNotOnBikeMoving: TRUE when the player asks to go fast, i.e. B is held, or with
// CE_QOL_AUTO_RUN, B is not held.
bool32 CeOverworld_IsRunButtonHeld(u16 heldKeys);

// PlayerNotOnBikeMoving, surfing: one step at running speed, or Mach Bike speed with
// CE_QOL_FAST_SURF while the run button (above) is held.
void CeOverworld_SurfStep(enum Direction direction, u16 heldKeys);

// Overworld_IsBikingAllowed, on a map whose header bans cycling: CE_QOL_BIKE_INDOORS and an
// ordinary indoor map (not a facility, link room, contest hall, ship or secret base).
bool32 CeOverworld_IsBikeIndoorsAllowed(void);

// ItemUseOutOfBattle_Bike: the player rides on a map CE_QOL_BIKE_INDOORS could have allowed,
// so getting off works there even after the feature is switched off mid-ride.
bool32 CeOverworld_CanGetOffBikeIndoors(void);

// ShouldDisableRandomEncounters: no step encounters (grass, water, caves), except in the
// Battle Pike and Battle Pyramid, whose challenges are built on them.
bool32 CeOverworld_NoWildEncounters(void);

// CheckForTrainersWantingBattle: trainers never spot the player, except in the Battle
// Pyramid and Trainer Hill. Talking to a trainer still starts the battle.
bool32 CeOverworld_NoTrainerSight(void);

// ScriptShowItemDescription: TRUE to draw the description box. It is skipped while another
// window uses the box's tiles (the money box when Mt. Chimney and the Route 109 Seashore House
// give an item), so nothing is drawn over it.
bool32 CeOverworld_ShowItemDescription(void);

// ScriptHideItemDescription: TRUE when CeOverworld_ShowItemDescription drew a box that is still
// up. This, not the feature's gate, decides, so a box is never removed twice or left behind.
bool32 CeOverworld_HideItemDescription(void);

#endif // GUARD_CE_OVERWORLD_H
