#ifndef GUARD_CE_KEY_ITEMS_H
#define GUARD_CE_KEY_ITEMS_H

// QoL key items. Having the item in the bag is the feature: CeKeyItems_Sync gives it while
// its feature is on and takes it back while it is off, so nothing else reads the gate.
//   ITEM_CE_CANDY_JAR       CE_QOL_CANDY_JAR       a Rare Candy that is never used up
//   ITEM_CE_MOVE_RELEARNER  CE_QOL_RELEARNER_ITEM  the Fallarbor relearner (level-up moves), free
//   ITEM_CE_MOVE_DELETER    CE_QOL_DELETER_ITEM    the Lilycove move deleter
//   ITEM_POKEMON_BOX_LINK   CE_QOL_PC_ITEM         upstream's Pokémon Storage System item
//   ITEM_CE_INFINITE_REPEL  CE_QOL_INFINITE_REPEL  a Repel the player switches on and off
// The new items' gItemsInfo entries are in ce/key_items_info.h; their scripts are in
// data/scripts/ce/key_items.inc. Hooks in upstream code:
//   src/data/items.h  gItemsInfo                    the entries above
//   data/event_scripts.s                            the scripts
//   src/item.c        AddBagItem, CheckBagHasSpace  CeKeyItems_MakeRoomFor
//   src/item_use.c    ItemUseOutOfBattle_RareCandy  CeKeyItems_BlockPartyItemUse
//   src/wild_encounter.c UpdateRepelCounter         CeKeyItems_IsInfiniteRepelOn

// Give the key items of enabled features and take back those of disabled ones. Idempotent;
// called at New Game, on Continue and when the options menu closes in-game. Skipped while
// the Battle Pyramid bag stands in for the bag. A full Key Items pocket only delays a gift
// to a later call.
void CeKeyItems_Sync(void);

// AddBagItem and CheckBagHasSpace, normal bag only: when another Key Items pocket item has
// no room, put QoL key items away (the next CeKeyItems_Sync gives them back once there is
// room), so the QoL items can never keep the player from receiving a story item.
void CeKeyItems_MakeRoomFor(u16 itemId, u16 count);

// ItemUseOutOfBattle_RareCandy: TRUE (after telling the player) when the party is empty.
// Only the Candy Jar reaches this with an empty party, through the Select button; from the
// bag, ItemMenu_UseOutOfBattle already refuses party-menu items without a Pokémon.
bool32 CeKeyItems_BlockPartyItemUse(u8 taskId);

// gItemsInfo fieldUseFunc of ITEM_CE_MOVE_RELEARNER and ITEM_CE_MOVE_DELETER, from the bag
// or registered to Select.
void CeItemUse_MoveRelearner(u8 taskId);
void CeItemUse_MoveDeleter(u8 taskId);

// The Infinite Repel is upstream's Repel with a step count no item gives: while it is on,
// VAR_REPEL_STEP_COUNT holds CE_INFINITE_REPEL_STEPS, which IsWildLevelAllowedByRepel reads
// as a Repel in effect and UpdateRepelCounter (hooked) never counts down. Using a Lure
// replaces it, as one Repel replaces another; CeKeyItems_Sync switches it off along with
// its feature.
#define CE_INFINITE_REPEL_STEPS 0x7FFF
bool32 CeKeyItems_IsInfiniteRepelOn(void);

// gItemsInfo fieldUseFunc of ITEM_CE_INFINITE_REPEL: switch it on or off.
void CeItemUse_InfiniteRepel(u8 taskId);

extern const u8 CE_EventScript_MoveRelearnerItem[];
extern const u8 CE_EventScript_MoveDeleterItem[];

#endif // GUARD_CE_KEY_ITEMS_H
