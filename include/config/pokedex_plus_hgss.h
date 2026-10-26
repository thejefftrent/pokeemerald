#ifndef GUARD_CONFIG_POKEDEX_PLUS_HGSS_H
#define GUARD_CONFIG_POKEDEX_PLUS_HGSS_H

#define POKEDEX_PLUS_HGSS                   (CeQol_IsOn(CE_QOL_POKEDEX_PLUS)) // CE: was FALSE; runtime QoL switch, read only in C control flow (ce/qol.h, ce/pokedex.h)
#define HGSS_DECAPPED                       FALSE // If TRUE, uses decapped gfx and strings.
#define HGSS_DARK_MODE                      FALSE // If TRUE, enables dark mode.
#define HGSS_HIDE_UNSEEN_EVOLUTION_NAMES    TRUE  // CE: was FALSE; unseen evolutions show as ??? (ce/pokedex.h)
#define HGSS_HIDE_UNOWNED_EVOLUTION_METHODS FALSE // If TRUE, hides evolution methods.
#define HGSS_SORT_TMS_BY_NUM                FALSE // If TRUE, sorts the TMS in HGSS Dex by TM number, rather than alphabetically.
#define HGSS_SHOW_EGG_MOVES_FOR_EVOS        FALSE // CE: keep FALSE; its walk to the first pre-evolution never ends on looping edited evolutions

#endif // GUARD_CONFIG_POKEDEX_PLUS_HGSS_H
