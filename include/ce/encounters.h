#ifndef GUARD_CE_ENCOUNTERS_H
#define GUARD_CE_ENCOUNTERS_H

#include "constants/ce_encounters.h"

// Gift, static, starter, roamer, egg and in-game trade encounters (ce/encounters.json).
// The website rewrites gCeEncounters; game code reads it only through the functions below
// and the script macros in asm/macros/ce_encounters.inc.

struct CeEncounter
{
    u16 species;
    u8 level;  // 0 where the game ignores it (eggs, trades)
    u8 flags;  // CE_ENC_FLAG_*
    u16 item;  // held item
};

// Read only by the website: where each encounter happens and how the game uses it.
struct CeEncounterInfo
{
    u16 map;                       // MAP_* constant
    enum CeEncounterKind kind:8;
    u8 trade;                      // INGAME_TRADE_* for trade and trade-request rows
};

extern const struct CeEncounter gCeEncounters[CE_ENC_COUNT];
extern const struct CeEncounterInfo gCeEncounterInfo[CE_ENC_COUNT];

u16 CeEnc_Species(enum CeEncounterId id);
u8 CeEnc_Level(enum CeEncounterId id);
u16 CeEnc_Item(enum CeEncounterId id);
void CeEnc_ApplyFlags(struct Pokemon *mon, enum CeEncounterId id);

// What the script macros do, callable from C (the self-test uses these).
void CeEnc_LoadVars(enum CeEncounterId id);
void CeEnc_ApplyFlagsToGiven(enum CeEncounterId id, u32 giveResult);
void CeEnc_CreateWildMon(enum CeEncounterId id);
void CeEnc_CreateEventMon(enum CeEncounterId id);

// Starters (slot 0-2: grass, fire, water).
u16 CeEnc_Starter(u32 slot);
void CeEnc_GiveStarter(u16 species, u32 slot);

// In-game trades: vanilla values in, gCeEncounters values out (unchanged for trades not in the table).
u16 CeEnc_TradeSpecies(u32 trade, u16 vanillaSpecies);
u16 CeEnc_TradeRequest(u32 trade, u16 vanillaRequest);
const u8 *CeEnc_TradeNickname(u32 trade, u16 vanillaSpecies, const u8 *vanillaNickname);
void CeEnc_FinishTradeMon(struct Pokemon *mon, u32 trade, u16 vanillaItem);

#endif // GUARD_CE_ENCOUNTERS_H
