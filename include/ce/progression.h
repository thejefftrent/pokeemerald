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
//   data/scripts/cable_club.inc CableClub_OnTransition          CE_QOL_ROOM_SELLERS, CE_QOL_BATTLE_ITEMS,
//                                                               CE_QOL_TUTOR_SELLER
//   data/maps/LittlerootTown_BrendansHouse_2F, _MaysHouse_2F  OnTransition  (hides the old
//                                                               bedroom sellers' placeholders)
//   src/battle_util.c           GetAttackerObedienceForAction   CE_QOL_ALWAYS_OBEY
//   src/battle_util.c           IsGen6ExpShareEnabled           CE_QOL_EXP_SHARE_ALL
//   src/battle_main.c           ClearSetBScriptingStruct        CE_QOL_CATCH_EXP
//   src/battle_script_commands.c SetBallThrowShakes             CE_QOL_ALWAYS_CATCH
//   src/pokemon_summary_screen.c ShouldShowIvEvPrompt           CE_QOL_SUMMARY_IV_EV
//   src/pokemon_summary_screen.c ShouldShowMoveRelearner        CE_QOL_SUMMARY_RELEARNER
//   src/daycare.c               TryToHatchEgg                   CE_QOL_FAST_EGG_HATCHING
//   src/pokemon.c               ComputePlayerShinyOdds          CE_QOL_SHINY_CHARM
// Nothing is cached outside a battle or a menu that is open, so a feature flipped from the
// options menu takes effect the next time the bag, a mart, a summary or a battle opens.

// SetBallThrowShakes: with CE_QOL_ALWAYS_CATCH every ball that is thrown at a wild Pokémon
// catches it, as a Master Ball does (3 shakes, or the critical-capture throw for an owned species
// under B_CRITICAL_CAPTURE_IF_OWNED). The catch chance is still computed and recorded for tests,
// but no shake or critical-capture random number is drawn. Whatever refuses a ball runs earlier
// and still does: trainers, the ghost and Wally's tutorial in Cmd_handleballthrow, and the catch
// gates when the ball is chosen (GetBallThrowableState in src/item_use.c: two Pokémon out, no
// room, a semi-invulnerable target, the bag rule and the game modes' catch rules). Battles the
// modes exempt (the Safari Zone, Birch's rescue) have no gates, so there a catch is certain.

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
// Mega Stones in Mossdeep's. Battle items have no town mart: only their Pokémon Center seller
// sells them. The list is on the heap and TryFreeDynamicShopItemList frees it.
void CeProgression_BuildShopItemList(const u16 **itemList, u16 *itemCount);

// The QoL shops' stock (ITEM_NONE-terminated). The town marts above add some, and the sellers
// upstairs in every Pokémon Center sell each whole (data/scripts/ce/pc_sellers.inc):
//   gCeShopNatureMints         CE_QOL_NATURE_MINTS, the Lavaridge Herb Shop only (21 mints)
//   gCeShopTrainingItems       CE_QOL_NATURE_MINTS, the training seller: the mints, Ability
//                              Capsule and Patch, the six vitamins and the six Power items
//   gCeShopEvolutionItems      CE_QOL_TRADE_EVOS_WITHOUT_TRADE (Mauville adds the Linking Cord only)
//   gCeShopMegaStones          CE_QOL_MEGA_STONES, with the Mega Ring
//   gCeShopZCrystals           CE_QOL_Z_CRYSTALS, with the Z-Power Ring
//   gCeShopGemsPlatesMemories  CE_QOL_GEMS_PLATES_MEMORIES
//   gCeShopBattleItems         CE_QOL_BATTLE_ITEMS, the battle items seller only
extern const u16 gCeShopNatureMints[];
extern const u16 gCeShopTrainingItems[];
extern const u16 gCeShopEvolutionItems[];
extern const u16 gCeShopMegaStones[];
extern const u16 gCeShopZCrystals[];
extern const u16 gCeShopGemsPlatesMemories[];
extern const u16 gCeShopBattleItems[];

// The sellers upstairs in every Pokémon Center, hidden by FLAG_TEMP_5..FLAG_TEMP_C (temp flags,
// which every map load clears first). On the 2F (onPcSecondFloor), a seller stands while its
// feature is on and, for the five shop sellers a town mart also stocks, CE_QOL_ROOM_SELLERS is
// on too; the battle items seller needs only CE_QOL_BATTLE_ITEMS, and the move tutor (egg and
// tutor moves, ce/relearner.h) only CE_QOL_TUTOR_SELLER. Everything else, and every seller
// anywhere else, gets its hide flag.
void CeProgression_UpdatePcSellers(bool32 onPcSecondFloor);
// CableClub_OnTransition (all 17 maps on LAYOUT_POKEMON_CENTER_2F) and the bedrooms'
// OnTransition, whose placeholders it always hides (callnative, requests_effects=1).
struct ScriptContext;
void CeProgression_ScriptUpdatePcSellers(struct ScriptContext *ctx);
// The self-test's view of the seller table: the index-th seller's hide flag, feature
// (CE_QOL_COUNT for a placeholder) and whether it also needs CE_QOL_ROOM_SELLERS. FALSE past
// the last seller.
bool32 CeProgression_GetPcSeller(u32 index, u16 *flag, u32 *feature, bool32 *needsCenterSellers);

// TryToHatchEgg: steps per Egg cycle. With CE_QOL_FAST_EGG_HATCHING a cycle takes a quarter of
// the steps, so every Egg hatches four times as fast (Flame Body still halves it again).
u32 CeProgression_EggCycleLength(u32 steps);

// ComputePlayerShinyOdds: CE_QOL_SHINY_CHARM gives the Shiny Charm's extra rolls
// (I_SHINY_CHARM_ADDITIONAL_ROLLS) without the item, so a Shiny Charm found or given
// elsewhere still works the same and is never taken away.

#endif // GUARD_CE_PROGRESSION_H
