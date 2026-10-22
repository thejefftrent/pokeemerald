#ifndef GUARD_CE_SPEED_H
#define GUARD_CE_SPEED_H

#include "ce/qol.h"

// Text and battle speed QoL. Most features are switched where upstream reads its config:
//   include/config/text.h    TEXT_SPEED_INSTANT          -> CE_QOL_INSTANT_TEXT
//   include/config/battle.h  B_FAST_INTRO_PKMN_TEXT,
//                            B_FAST_INTRO_NO_SLIDE       -> CE_QOL_FAST_BATTLE_INTRO
//                            B_FAST_HP_DRAIN             -> CE_QOL_FAST_HP_BAR
//                            B_FAST_EXP_GROW             -> CE_QOL_FAST_EXP_BAR
//                            B_QUICK_MOVE_CURSOR_TO_RUN  -> CE_QOL_QUICK_RUN_CURSOR
//                            B_RUN_TRAINER_BATTLE        -> CE_QOL_FORFEIT_TRAINER_BATTLES
//   include/config/save.h    SKIP_SAVE_CONFIRMATION      -> CE_QOL_QUICK_SAVE
// Those macros are only read in C control flow; a new #if on one of them fails to compile.
//
// All of them are off in a link battle, so both GBAs keep vanilla timing and turn flow
// whatever each player picked. Nothing here can change during a battle: the options menu
// cannot be opened there, and the gates read only the ROM config, the save and the battle type.

// Battle code (and code run right after a battle): CeQol_IsOn, but FALSE in a link battle.
bool32 CeSpeed_IsOnInBattle(enum CeQolFeature feature);

// B_FAST_INTRO_NO_SLIDE. Upstream compiles it FALSE, so TESTING builds keep the slide.
bool32 CeSpeed_NoIntroSlide(void);

// TEXT_SPEED_INSTANT: every text printer draws a page per frame (not in a link battle).
bool32 CeSpeed_IsInstantText(void);

// RenderText: holding A/B speeds up a print that A/B may speed up, without a fresh press.
bool32 CeSpeed_IsHoldToSpeedText(void);

// Cmd_waitmessage / Cmd_pause: the frames to wait (halved by CE_QOL_SHORT_BATTLE_PAUSES).
u16 CeSpeed_BattleWait(u16 frames);

// CB2_InitCopyrightScreenAfterBootup, once the save is loaded: with CE_QOL_SKIP_INTRO,
// go from the copyright screen straight to the title screen.
void CeSpeed_MaybeSkipBootIntro(void);

#endif // GUARD_CE_SPEED_H
