#ifndef GUARD_CONSTANTS_CE_TEAM_H
#define GUARD_CONSTANTS_CE_TEAM_H

// What CeTeam_DeliverNext handed out (VAR_RESULT in data/scripts/ce/team.inc). See ce/team.h.
#define CE_DELIVERY_NONE         0 // nothing (more) is waiting
#define CE_DELIVERY_MON          1 // a Pokémon arrived in its own slot
#define CE_DELIVERY_MON_MOVED    2 // a Pokémon arrived, in another slot because its own was taken
#define CE_DELIVERY_MON_NO_ROOM  3 // a Pokémon is waiting, but every box is full
#define CE_DELIVERY_ITEM         4 // an item arrived in the item PC
#define CE_DELIVERY_ITEM_NO_ROOM 5 // an item is waiting, but the item PC is full

#endif // GUARD_CONSTANTS_CE_TEAM_H
