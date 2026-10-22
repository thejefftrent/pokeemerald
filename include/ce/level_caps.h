#ifndef GUARD_CE_LEVEL_CAPS_H
#define GUARD_CE_LEVEL_CAPS_H

// Level caps tied to the next boss the player has not beaten (CE_QOL_LEVEL_CAPS).
// The cap is the highest level in the parties of the bosses up to and including that
// one, read at runtime from gTrainers, so caps follow edited or randomized trainers.

// The cap from progress alone, whatever the feature's mode. MAX_LEVEL once every boss
// is beaten. *bossOut (optional) gets the next boss's trainer id, or TRAINER_NONE.
u32 CeLevelCap_Compute(u16 *bossOut);

// The cap in force: CeLevelCap_Compute() when the feature is on, else MAX_LEVEL.
// This is what upstream's GetCurrentLevelCap() returns.
u32 CeLevelCap_GetEffective(void);

// Unlimited level items (the Candy Jar key item) are never consumed.
bool32 CeLevelCap_IsUnlimitedItem(u16 item);

// TRUE if using a level-raising item on a Pokémon of this level must be refused.
bool32 CeLevelCap_BlocksLevelItem(u16 item, u32 level);

// The boss list in force (the config's, or the built-in one); returns its length.
u32 CeLevelCap_GetBosses(const u16 **bosses);

// Clamp a level reached by gaining exp (daycare) to the cap, never below where it started.
u32 CeLevelCap_ClampGainedLevel(u32 before, u32 after);

#endif // GUARD_CE_LEVEL_CAPS_H
