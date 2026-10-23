#include "global.h"
#include "constants/event_bg.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "ce/items.h"

// Read only by the website and the self-test: where the map scripts give items and open
// Poké Marts (see include/ce/items.h).
#include "../data/ce_field_items.h"

extern const u8 Common_EventScript_FindItem[];

const struct CeItemScriptInfo gCeItemScripts =
{
    .sites = gCeItemScriptSites,
    .owners = gCeItemScriptOwners,
    .findItemScript = Common_EventScript_FindItem,
    .siteCount = ARRAY_COUNT(gCeItemScriptSites),
    .ownerCount = ARRAY_COUNT(gCeItemScriptOwners),
    .hiddenItemFlagsStart = FLAG_HIDDEN_ITEMS_START,
    .bgEventHiddenItem = BG_EVENT_HIDDEN_ITEM,
    .opSetOrCopyVar = CE_ITEM_SCRIPT_OP_SETORCOPYVAR,
    .opCallStd = CE_ITEM_SCRIPT_OP_CALL_STD,
    .opPokemart = CE_ITEM_SCRIPT_OP_POKEMART,
    .stdObtainItem = CE_ITEM_SCRIPT_STD_OBTAIN_ITEM,
};
