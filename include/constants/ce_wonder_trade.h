#ifndef GUARD_CONSTANTS_CE_WONDER_TRADE_H
#define GUARD_CONSTANTS_CE_WONDER_TRADE_H

// Wonder Trade (ce/wonder_trade.h): the codes data/scripts/ce/wonder_trade.inc branches on.

// CeWonderTrade_Block: whether the attendant trades at all (VAR_RESULT of the gate).
#define CE_WT_OPEN                0 // trades
#define CE_WT_REFUSED_OFF         1 // CE_QOL_WONDER_TRADE is off (the player switched it off)
#define CE_WT_REFUSED_RULES       2 // a Nuzlocke or single-Pokémon run
#define CE_WT_REFUSED_STATIC_TEAM 3 // a static team run
#define CE_WT_REFUSED_EMPTY_POOL  4 // no species can come (a monotype type nothing has)

// CeWonderTrade_CheckMon: whether the chosen Pokémon can be sent (VAR_RESULT of the check).
#define CE_WT_OK          0 // it can
#define CE_WT_EGG         1 // an Egg
#define CE_WT_CANT_TRADE  2 // its species can't be traded (fused Kyurem, Necrozma, Calyrex...)
#define CE_WT_MAIL        3 // it holds Mail
#define CE_WT_BAG_FULL    4 // it holds an item the bag has no room for
#define CE_WT_LAST_SURFER 5 // the last Pokémon that knows Surf, and Surf needs it
#define CE_WT_LAST_MON    6 // the party's last Pokémon that can battle
#define CE_WT_DEAD        7 // dead in a Nuzlocke

#endif // GUARD_CONSTANTS_CE_WONDER_TRADE_H
