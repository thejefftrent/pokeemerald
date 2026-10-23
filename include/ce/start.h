#ifndef GUARD_CE_START_H
#define GUARD_CE_START_H

// New-game inventory and money (settings.start). The website writes gCeRomConfig.body.start;
// CeStart_OnNewGame applies it at the end of NewGameInitData (hook in src/new_game.c), before
// the QoL key items are given. This header is included from ce/rom_config.h, so it only uses
// basic types.
//
// Every field is 0 for vanilla: an empty bag, one Potion in the PC and ¥3000.

#define CE_START_BAG_ITEMS 48
#define CE_START_PC_ITEMS  16

struct CeStartItem
{
    u16 item;     // ITEM_NONE ends the list
    u16 quantity; // 1-999 (MAX_BAG_ITEM_CAPACITY, MAX_PC_ITEM_CAPACITY)
};

struct CeStartConfig
{
    u8 setMoney;       // 0 = vanilla money (¥3000); 1 = money below
    u8 replacePcItems; // 0 = vanilla PC items (one Potion); 1 = pc below, even if empty
    u16 padding;
    u32 money;         // at most MAX_MONEY
    struct CeStartItem bag[CE_START_BAG_ITEMS];
    struct CeStartItem pc[CE_START_PC_ITEMS];
};

// End of NewGameInitData: set the money, replace the PC items and add the bag items. An item
// that doesn't fit its pocket is skipped; the website checks pocket sizes before writing.
void CeStart_OnNewGame(void);

#endif // GUARD_CE_START_H
