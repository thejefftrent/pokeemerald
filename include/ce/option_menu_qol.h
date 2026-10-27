#ifndef GUARD_CE_OPTION_MENU_QOL_H
#define GUARD_CE_OPTION_MENU_QOL_H

// The QoL page of the options menu (press R) and competitive locks on vanilla rows.
// Hooked from src/option_menu.c; uses option-menu task data[7..10] and no other RAM.

#include "ce/qol.h"

// CB2_InitOptionMenu, once the task holds the saved values: show locked values, draw the R hint.
void CeOptions_OnMenuInit(u8 taskId, s16 *textSpeed, s16 *battleSceneOff, s16 *battleStyle);
// First thing in Task_OptionMenuProcessInput. TRUE = handled, skip the vanilla code.
bool32 CeOptions_ProcessInput(u8 taskId);
// First thing in Task_OptionMenuSave: keep the player's own values for locked rows.
void CeOptions_OnMenuSave(u8 taskId, s16 *textSpeed, s16 *battleSceneOff, s16 *battleStyle);
// DrawOptionMenuChoice, after its colour patch: grey out choices on locked rows.
void CeOptions_GreyLockedChoice(u8 *dst, u32 row, u32 selected);
// DrawOptionMenuTexts, after the labels: grey out the labels of locked rows.
void CeOptions_GreyLockedLabels(u32 windowId, const u8 *const *names);
// Whether the QoL page lists this feature: not locked off (unless the ROM shows those), and not
// refused by the run's rules (Wonder Trade in Nuzlocke, single-Pokémon and static team runs).
bool32 CeOptions_IsQolRowShown(enum CeQolFeature feature);

#endif // GUARD_CE_OPTION_MENU_QOL_H
