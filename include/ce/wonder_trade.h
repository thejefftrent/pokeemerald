#ifndef GUARD_CE_WONDER_TRADE_H
#define GUARD_CE_WONDER_TRADE_H

#include "constants/ce_wonder_trade.h"
#include "constants/flags.h"

// Wonder Trade (CE_QOL_WONDER_TRADE, docs/wonder-trade.md): an attendant upstairs in every Pokémon
// Center (the FLAG_TEMP_C seller of sPcSellers, src/ce/progression.c, whose object is at the end of
// each 2F map.json) trades one Pokémon from the party or the PC for a random one at the same level,
// as often as the player likes and for free.
//
// The received Pokémon comes from another trainer: a random OT id and one of a few OT names, met in
// a trade, random IVs (with its species' perfect IVs), normal shiny odds, its level-up moves. So it
// gets the traded-Pokémon Exp. boost and the badge obedience rule, as in-game trades do. It holds
// nothing. The sent Pokémon's held item goes back to the bag first.
//
// The pool: every enabled species that is a base form or a regional form (Alolan, Galarian,
// Hisuian, Paldean), legendaries and mythicals included; never a Mega, Primal, Ultra Burst,
// Gigantamax, Tera or totem form, a battle-only form (sBattleOnlyForms, a copy of the engine's
// BATTLE_ONLY_FORMS), a species that can't be traded, or an alternate or cosmetic form. Each
// Pokédex entry is equally likely; a regional form shares its entry's chance. In a monotype run
// only species of the run's type come (CeSpecies_IsOfType, as the catch rule: the third type
// counts).
//
// Refused, and the attendant hidden, in Nuzlocke, single-Pokémon and static team runs (the save's
// rules). Hard mode, no-heal, no-evolution, no-experience and monotype runs may trade.
//
// The trade plays upstream's in-game trade scene (DoInGameTradeScene, wireless animation): it
// swaps the Pokémon, sets the dex flags, sends a PC pick's new Pokémon to the PC and evolves a
// received trade evolution (unless the run has no evolutions). One-line hooks in src/trade.c:
//   BufferTradeSceneStrings   CeWonderTrade_BufferSceneStrings (the scene's names)
//   BufferInGameTradeMonName  CeWonderTrade_BufferMonName (after the scene)
// Both do nothing unless FLAG_CE_WONDER_TRADE is set, so the vanilla in-game trades are unchanged.
// Script: data/scripts/ce/wonder_trade.inc (#included by data/event_scripts.s).

// Special flag (RAM only, never saved): the trade scene is playing a Wonder Trade.
#define FLAG_CE_WONDER_TRADE (SPECIAL_FLAGS_START + 0x7E)

struct Pokemon;
struct ScriptContext;

// Why the attendant won't trade (CE_WT_REFUSED_...), or CE_WT_OPEN.
u32 CeWonderTrade_Block(void);
// The run's rules allow Wonder Trade: not Nuzlocke, single Pokémon or a static team. The
// sPcSellers row hides the attendant otherwise.
bool32 CeWonderTrade_IsAllowed(void);

// Whether `species` can come from Wonder Trade (TYPE_NONE: any type; otherwise only species of
// that type).
bool32 CeWonderTrade_InPool(u16 species, u32 type);
// The Pokédex entries (form groups: a species and its regional forms) with a species in the pool.
u32 CeWonderTrade_CountGroups(u32 type);
// The group of `species` (its form table's first species, or itself).
u16 CeWonderTrade_GroupOf(u16 species);
// A random species from the pool: an entry, then one of its species in the pool. SPECIES_NONE
// only if the pool is empty.
u16 CeWonderTrade_PickSpecies(u32 type);
// The battle-only forms the pool leaves out (the self-test compares them with the engine's).
u32 CeWonderTrade_BattleOnlyCount(void);
u16 CeWonderTrade_BattleOnlyForm(u32 i);

// The received Pokémon, as described above.
void CeWonderTrade_CreateMon(struct Pokemon *mon, u16 species, u8 level);

// The Pokémon chooseboxmon picked (VAR_0x8004, VAR_MON_BOX_ID/POS): CE_WT_OK or why it can't be
// sent. Fills gStringVar1 with its nickname and gStringVar2 with its held item's name.
u32 CeWonderTrade_CheckMon(void);

// After the scene, for a PC pick: upstream copies the received Pokémon to the PC before a trade
// evolution, so the box keeps the unevolved one; this copies the evolved one over it. Returns
// TRUE if it copied.
bool32 CeWonderTrade_FixPcEvolution(void);

// The trade.c hooks: FALSE (and nothing done) unless FLAG_CE_WONDER_TRADE is set.
bool32 CeWonderTrade_BufferSceneStrings(void);
bool32 CeWonderTrade_BufferMonName(void);

// Script natives (callnative, requests_effects=1) for data/scripts/ce/wonder_trade.inc.
// Gate: VAR_RESULT = CeWonderTrade_Block().
void CeWonderTrade_ScriptGate(struct ScriptContext *ctx);
// VAR_RESULT = CeWonderTrade_CheckMon().
void CeWonderTrade_ScriptCheckChosen(struct ScriptContext *ctx);
// Picks the species, moves the held item to the bag, creates the received Pokémon and sets
// FLAG_CE_WONDER_TRADE. VAR_RESULT: 0 nothing could come (nothing changed), 1 ready, 2 ready and
// the held item (gStringVar2) went to the bag.
void CeWonderTrade_ScriptPrepare(struct ScriptContext *ctx);
// After the scene: the PC write-back, gStringVar1 = the received Pokémon's nickname, gStringVar2
// = its OT, gStringVar3 = the box a PC pick's Pokémon went to; VAR_RESULT = TRUE for a PC pick.
// Clears FLAG_CE_WONDER_TRADE.
void CeWonderTrade_ScriptFinish(struct ScriptContext *ctx);

#endif // GUARD_CE_WONDER_TRADE_H
