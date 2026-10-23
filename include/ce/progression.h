#ifndef GUARD_CE_PROGRESSION_H
#define GUARD_CE_PROGRESSION_H

#include "ce/qol.h"
#include "constants/items.h"

// Pokémon and progression QoL. Each feature is switched by one-line hooks in upstream code:
//   src/item.c                  GetItemImportance               CE_QOL_REUSABLE_TMS
//   src/item.c                  GetItemType, GetItemFieldFunc   CE_QOL_TRADE_EVOS_WITHOUT_TRADE
//   src/shop.c                  CB2_InitBuyMenu                 CE_QOL_NATURE_MINTS,
//                                                               CE_QOL_TRADE_EVOS_WITHOUT_TRADE,
//                                                               CE_QOL_MEGA_STONES, CE_QOL_Z_CRYSTALS,
//                                                               CE_QOL_GEMS_PLATES_MEMORIES
//   data/maps/LittlerootTown_BrendansHouse_2F, _MaysHouse_2F  OnTransition  CE_QOL_ROOM_SELLERS
//   src/battle_util.c           GetAttackerObedienceForAction   CE_QOL_ALWAYS_OBEY
//   src/battle_util.c           IsGen6ExpShareEnabled           CE_QOL_EXP_SHARE_ALL
//   src/battle_main.c           ClearSetBScriptingStruct        CE_QOL_CATCH_EXP
//   src/pokemon_summary_screen.c ShouldShowIvEvPrompt           CE_QOL_SUMMARY_IV_EV
//   src/pokemon_summary_screen.c ShouldShowMoveRelearner        CE_QOL_SUMMARY_RELEARNER
//   src/daycare.c               TryToHatchEgg                   CE_QOL_FAST_EGG_HATCHING
//   src/pokemon.c               ComputePlayerShinyOdds          CE_QOL_SHINY_CHARM
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
// sells in this map's mart: nature mints in the Lavaridge Herb Shop, the Linking Cord in the
// Mauville Poké Mart, Gems, Plates and Memories in Slateport's, Z-Crystals in Fortree's and
// Mega Stones in Mossdeep's. The list is on the heap and TryFreeDynamicShopItemList frees it.
void CeProgression_BuildShopItemList(const u16 **itemList, u16 *itemCount);

// The QoL shops' stock (ITEM_NONE-terminated). The town marts above add them, and with
// CE_QOL_ROOM_SELLERS a seller in the player's room sells each (data/scripts/ce/room_sellers.inc):
//   gCeShopNatureMints         CE_QOL_NATURE_MINTS
//   gCeShopEvolutionItems      CE_QOL_TRADE_EVOS_WITHOUT_TRADE (Mauville adds the Linking Cord only)
//   gCeShopMegaStones          CE_QOL_MEGA_STONES, with the Mega Ring
//   gCeShopZCrystals           CE_QOL_Z_CRYSTALS, with the Z-Power Ring
//   gCeShopGemsPlatesMemories  CE_QOL_GEMS_PLATES_MEMORIES
extern const u16 gCeShopNatureMints[];
extern const u16 gCeShopEvolutionItems[];
extern const u16 gCeShopMegaStones[];
extern const u16 gCeShopZCrystals[];
extern const u16 gCeShopGemsPlatesMemories[];

// The bedrooms' OnTransition (callnative CeProgression_ScriptUpdateRoomSellers): a seller stands
// in the player's own bedroom while CE_QOL_ROOM_SELLERS and its feature are on; every other one
// (and all of them in the rival's room) gets its hide flag, a temp flag the next map load clears.
void CeProgression_UpdateRoomSellers(void);
struct ScriptContext;
void CeProgression_ScriptUpdateRoomSellers(struct ScriptContext *ctx);

// TryToHatchEgg: steps per Egg cycle. With CE_QOL_FAST_EGG_HATCHING a cycle takes a quarter of
// the steps, so every Egg hatches four times as fast (Flame Body still halves it again).
u32 CeProgression_EggCycleLength(u32 steps);

// ComputePlayerShinyOdds: CE_QOL_SHINY_CHARM gives the Shiny Charm's extra rolls
// (I_SHINY_CHARM_ADDITIONAL_ROLLS) without the item, so a Shiny Charm found or given
// elsewhere still works the same and is never taken away.

#endif // GUARD_CE_PROGRESSION_H
