#ifndef GUARD_CE_RULES_WARNING_H
#define GUARD_CE_RULES_WARNING_H

#include "ce/qol.h"

// The save keeps the challenge rules it started with (ce/modes.h), so a save loaded on a ROM
// patched with other rules plays on with the old ones. The main menu says so once, when the
// save loads (hook: Task_MainMenuCheckSaveFile, SAVE_STATUS_OK).
//
// The rules themselves are compared, not the config hash: the hash covers every setting, and
// re-patching may change QoL, the randomizer and the rest without touching the rules. QoL needs
// no warning: locked features always follow the ROM, and the player's own picks are kept.
//
// "Shown" is the last bit of the save's QoL value words, which no feature uses: a feature's
// value bit only counts while its chosen bit is set, and there is no feature CE_QOL_MAX - 1.
// It is set when the warning shows and cleared when a save loads on a ROM with its own rules,
// so going back to the original ROM and then to another one warns again. Like any change to
// the save, it lasts once the player saves.
#define CE_RULES_WARNED_BIT (CE_QOL_MAX - 1)

// TRUE if this ROM's rules differ from the save's (a save without rules has none).
bool32 CeRulesWarning_RulesDiffer(void);

// TRUE once per difference: the caller shows CeRulesWarning_Text().
bool32 CeRulesWarning_ShouldShow(void);

const u8 *CeRulesWarning_Text(void);

#endif // GUARD_CE_RULES_WARNING_H
