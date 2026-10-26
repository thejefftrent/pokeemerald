#ifndef GUARD_CE_POKEDEX_H
#define GUARD_CE_POKEDEX_H

#include "ce/qol.h"
#include "constants/species.h"

// Pokédex Plus (CE_QOL_POKEDEX_PLUS): expansion's HeartGold/SoulSilver-style Pokédex
// (src/pokedex_plus_hgss.c), switched at runtime. include/config/pokedex_plus_hgss.h defines
// POKEDEX_PLUS_HGSS as CeQol_IsOn(CE_QOL_POKEDEX_PLUS); every use of it is C control flow, read
// when a dex screen, the area screen or the battle's caught-Pokémon page starts. No RAM: the
// dex's struct PokedexView lives on the heap, and the HGSS code has no statics.
//
// The HGSS dex shows, for a caught species, its stats and moves (Stats), its evolution family
// with each method (Evos) and its other forms (Forms). It reads the ROM's own (website-edited)
// gSpeciesInfo evolutions, learnsets, egg moves and teachable bits.
//
// Spoilers on the Evos page: a species the player hasn't seen shows as ??? with the
// question-mark icon, wherever it appears (targets, pre-evolutions, the species a method names).
// Methods are always shown, so edited and randomized evolutions are explained. The cursor stops
// only on seen species.
//
// Hooks, one line each (// CE:):
//   include/config/pokedex_plus_hgss.h  POKEDEX_PLUS_HGSS (the switch),
//                                       HGSS_HIDE_UNSEEN_EVOLUTION_NAMES TRUE
//   src/pokedex.c, pokedex_area_screen.c, pokedex_plus_hgss.c  includes
//   src/pokedex.c          Task_ClosePokedex: sPokedexView = NULL after freeing it (the
//                          caught-Pokémon page in battle frees a non-NULL pointer again, a double
//                          free with either dex once the dex had been opened and closed)
//   src/pokedex_plus_hgss.c  the Evos page (bounds below, ??? names and ? icons, an emptied
//                          evolution list) and the Forms page (CE_DEX_FORM_ICONS)
//
// Bounds for edited data (the website can give a species any evolutions): at most
// CE_DEX_EVO_ROWS rows and CE_DEX_EVO_MAX_DEPTH levels below the viewed species (a loop of
// evolutions stops there), one pre-evolution row (two for Gholdengo, which upstream special-cases,
// only while two different species evolve into it; with fewer it gets the usual rows) and one
// grandparent row, and CE_DEX_FORM_ICONS alternate forms. Vanilla stays inside them
// except Eevee (10 evolution entries) and Milcery, which upstream wrote one row past the arrays,
// Burmy's three forms all evolving into Mothim, and Pikachu, Unown, Vivillon, Minior, Arceus,
// Silvally and Alcremie, whose forms upstream wrote past the task's data.
//
// Sprites: upstream's Evos and Forms pages free some icons from the wrong task-data slots, but
// every dex screen resets all sprites when it loads (ResetOtherVideoRegisters without OBJ), so
// nothing piles up; tools/canary/pokedex.ts counts the live sprites over 30 page cycles.
//
// HGSS_SHOW_EGG_MOVES_FOR_EVOS stays FALSE: its walk to the first pre-evolution
// (GetSelectedMove) never ends when edited evolutions loop.

// struct EvoScreenData's rows (arrowSpriteDist, targetSpecies, seen; pokedex_common.h).
#define CE_DEX_EVO_ROWS 10
// Evolution levels printed below the viewed species (vanilla needs 2).
#define CE_DEX_EVO_MAX_DEPTH 4
// Alternate-form icons on the Forms page: their sprite ids live in task data[5..15].
#define CE_DEX_FORM_ICONS (NUM_TASK_DATA - 5)

// Whether the player has seen the species (its national dex number's seen flag). FALSE for
// SPECIES_NONE, an id out of range and a species without a national dex number.
bool32 CeDex_IsSeen(enum Species species);
// The species if seen, else SPECIES_NONE (whose icon is the question mark).
enum Species CeDex_SeenOrUnknown(enum Species species);
// The species' name if seen, else "???".
const u8 *CeDex_SeenName(enum Species species);

#endif // GUARD_CE_POKEDEX_H
