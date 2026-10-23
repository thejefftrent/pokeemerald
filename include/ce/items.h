#ifndef GUARD_CE_ITEMS_H
#define GUARD_CE_ITEMS_H

// Items found in the world, for the website (packages/engine/src/items/). Nothing here is
// read by game code: the website edits the data the game already reads in place.
// - Item balls: ObjectEventTemplate.trainerRange_berryTreeId (item) and movementRangeX
//   (amount) of object events whose script is Common_EventScript_FindItem.
// - Hidden items: BgEvent.bgUnion.hiddenItem of BG_EVENT_HIDDEN_ITEM events.
// - Item gifts: the operands of giveitem's two setorcopyvar commands.
// - Poké Marts: the list pointer of the pokemart command (new lists go to free space).
// tools/ce/gen_field_items.py lists the giveitem and pokemart commands below, and which map
// events reach them, at build time.

enum CeItemSiteKind
{
    CE_ITEM_SITE_GIFT,  // giveitem ITEM_X, n
    CE_ITEM_SITE_MART,  // pokemart List
};

enum CeItemOwnerKind
{
    CE_ITEM_OWNER_OBJECT,      // index into the map's object events
    CE_ITEM_OWNER_BG,          // index into the map's bg events (signs)
    CE_ITEM_OWNER_COORD,       // index into the map's coord events (triggers)
    CE_ITEM_OWNER_MAP_SCRIPT,  // the map's own scripts (on transition, on frame, ...)
};

// One giveitem or pokemart command: the nth + 1-th match between anchor and limit of
// - a gift: setorcopyvar VAR_0x8000 <item>, setorcopyvar VAR_0x8001 <amount>, callstd STD_OBTAIN_ITEM
// - a mart: pokemart <pointer to an ITEM_NONE-terminated list of items>
// where only the bytes outside <> must match, so edited commands are still found.
struct CeItemScriptSite
{
    const u8 *anchor;        // the nearest global script label above the command
    const u8 *limit;         // the next global label below it, or NULL
    const char *key;         // stable name: the label the command is under, "#n" for the n-th
    const u16 *martItems;    // marts: the vanilla list (ITEM_NONE-terminated)
    u16 item;                // gifts: the vanilla item
    u16 amount;              // gifts: the vanilla amount
    enum CeItemSiteKind kind:8;
    u8 nth;                  // giveitem (or pokemart) commands between anchor and this one
    u16 ownerStart;          // owners: gCeItemScriptOwners[ownerStart .. ownerStart + ownerCount)
    u16 ownerCount;
};

// A map event whose script reaches a command.
struct CeItemScriptOwner
{
    u16 map;                 // MAP_* constant
    enum CeItemOwnerKind kind:8;
    u8 index;
};

struct CeItemScriptInfo
{
    const struct CeItemScriptSite *sites;
    const struct CeItemScriptOwner *owners;
    const u8 *findItemScript;     // object events with this script are item balls
    u16 siteCount;
    u16 ownerCount;
    u16 hiddenItemFlagsStart;     // FLAG_HIDDEN_ITEMS_START
    u8 bgEventHiddenItem;         // BG_EVENT_HIDDEN_ITEM
    u8 opSetOrCopyVar;            // script opcodes and the std script giveitem calls
    u8 opCallStd;
    u8 opPokemart;
    u8 stdObtainItem;
};

extern const struct CeItemScriptInfo gCeItemScripts;

#endif // GUARD_CE_ITEMS_H
