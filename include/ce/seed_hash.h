#ifndef GUARD_CE_SEED_HASH_H
#define GUARD_CE_SEED_HASH_H

// The seed hash: five Pokémon the website picks from the settings and seed
// (gCeRomConfig.hdr.seedHash), shown as icons on the main menu so players can check they
// have the same ROM. A base ROM has no hash and shows nothing.

// Whether this ROM has a complete hash (every entry a valid species).
bool32 CeSeedHash_IsPresent(void);

// Main menu: the icons in the strip below the menu, when it has room (the NEW GAME and
// CONTINUE layouts; the longer Mystery Gift menus fill the screen). The sprites are freed
// with everything else when the next screen resets sprite data.
void CeSeedHash_ShowOnMainMenu(u32 menuItemCount);

#endif // GUARD_CE_SEED_HASH_H
