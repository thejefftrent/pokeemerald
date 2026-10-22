#ifndef GUARD_CE_PROGRESSION_H
#define GUARD_CE_PROGRESSION_H

#include "ce/qol.h"
#include "constants/items.h"

// Pokémon and progression QoL. Each feature is switched by one-line hooks in upstream code:
//   src/item.c                  GetItemImportance               CE_QOL_REUSABLE_TMS
//   src/item.c                  GetItemType, GetItemFieldFunc   CE_QOL_TRADE_EVOS_WITHOUT_TRADE
//   src/shop.c                  CB2_InitBuyMenu                 CE_QOL_NATURE_MINTS,
//                                                               CE_QOL_TRADE_EVOS_WITHOUT_TRADE
//   src/battle_util.c           GetAttackerObedienceForAction   CE_QOL_ALWAYS_OBEY
//   src/battle_util.c           IsGen6ExpShareEnabled           CE_QOL_EXP_SHARE_ALL
//   src/battle_main.c           ClearSetBScriptingStruct        CE_QOL_CATCH_EXP
//   src/pokemon_summary_screen.c ShouldShowIvEvPrompt           CE_QOL_SUMMARY_IV_EV
//   src/pokemon_summary_screen.c ShouldShowMoveRelearner        CE_QOL_SUMMARY_RELEARNER
// Nothing is cached outside a battle or a menu that is open, so a feature flipped from the
// options menu takes effect the next time the bag, a mart, a summary or a battle opens.

// GetItemImportance: with CE_QOL_REUSABLE_TMS every TM is important, as with upstream's
// I_REUSABLE_TMS. Teaching does not use it up; it shows no quantity, cannot be sold, tossed,
// deposited, held or Flung, and a mart sells it once (SOLD OUT while one is owned).
bool32 CeProgression_IsReusableTm(enum Item itemId);

// GetItemType / GetItemFieldFunc: with CE_QOL_TRADE_EVOS_WITHOUT_TRADE the held items that
// trade evolutions need (Metal Coat, King's Rock, Dragon Scale, Up-Grade, ...) are used from
// the bag like an evolution stone, as with upstream's I_USE_EVO_HELD_ITEMS_FROM_BAG. The
// species data already has the matching EVO_ITEM entries.
bool32 CeProgression_IsBagEvoItem(enum Item itemId);

// TRUE inside Battle Frontier challenges where moves must not change (Pike, Pyramid,
// Tower multi partner room).
bool32 CeProgression_InFrontierChallenge(void);

// CB2_InitBuyMenu, normal marts: upstream's dynamic list, plus the items an enabled feature
// sells in this map's mart (nature mints in the Lavaridge Herb Shop, the Linking Cord in the
// Mauville Poké Mart). The list is on the heap and TryFreeDynamicShopItemList frees it.
void CeProgression_BuildShopItemList(const u16 **itemList, u16 *itemCount);

#endif // GUARD_CE_PROGRESSION_H
