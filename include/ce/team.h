#ifndef GUARD_CE_TEAM_H
#define GUARD_CE_TEAM_H

// Static team challenge: the player's Pokémon are set by the website, and nothing new can join.
//
// The website writes the team into gCeTeamMons (ce/team_table.h): a starting party and
// Pokémon for chosen PC box slots, each with a number of gym badges. New Game gives the party
// and every PC Pokémon that needs no badge. The others are delivered when a PC is switched on
// with enough badges: into their own slot, or the first free one if it's taken, with a
// message. PC items from settings.start can wait for badges the same way (CeStartConfig.pcBadges).
// Until it arrives a Pokémon doesn't exist in the save, so there is nothing to withdraw, move,
// release or take an item from early.
//
// With the static team on, nothing can join the team: Poké Balls are refused (through the
// game modes' catch gate), the Safari Zone turns the player away, gift Pokémon and eggs are
// refused, the Day Care never finds an egg and in-game trades are declined. Birch's bag still
// opens and the pick still decides the rival's team (VAR_STARTER_MON), but the starter isn't
// given: the static team's lead fights the Zigzagoon.
//
// This header is included from global.h (for the save struct), so it only uses basic types.

#include "constants/ce_team.h"

struct ScriptContext;

#define CE_TEAM_MAX_MONS 40   // party and PC Pokémon together (gCeTeamMons)
#define CE_TEAM_IN_PARTY 0xFF // CeTeamMon.box of a party Pokémon
#define CE_TEAM_MAX_BADGES 8

// gCeRomConfig.body.team. Zero means vanilla.
struct CeTeamConfig
{
    u8 staticTeam;   // 1 = only the team: no catching, gifts, eggs or in-game trades
    u8 reserved[3];
};

// Spare SaveBlock2 space (struct Pokedex's old dex-flag filler, after ceModes). Saves made
// before this existed have zeros here: no team and no deliveries.
#define CE_TEAM_SAVE_MAGIC   0xC7
#define CE_TEAM_SAVE_VERSION 1

struct CeTeamSave
{
    u8 magic;            // CE_TEAM_SAVE_MAGIC once New Game set up a team or timed PC items
    u8 version;          // CE_TEAM_SAVE_VERSION
    u8 staticTeam;       // copy of CeTeamConfig.staticTeam from the ROM that started the run
    u8 reserved;
    u16 itemsDelivered;  // bit i = gCeRomConfig.body.start.pc[i] is in the PC
    u16 padding;
    u32 monsDelivered[2]; // bit i = gCeTeamMons[i] was given
};

bool32 CeTeam_SaveIsSetUp(void);
// Static team rules of this run (the save's copy).
bool32 CeTeam_IsStaticTeam(void);
// Nothing new may join: catches (outside Wally's tutorial), gifts, eggs and trades.
bool32 CeTeam_RefusesNewMons(void);
bool32 CeTeam_BlocksCatching(void);
const u8 *CeTeam_CatchBlockedText(void);

// Gym badges the player has (FLAG_BADGE01_GET..FLAG_BADGE08_GET).
u32 CeTeam_BadgeCount(void);

// End of NewGameInitData, after the starting items: the party, the PC Pokémon and items that
// need no badge, and the save's copy of the rules.
void CeTeam_OnNewGame(void);

// Hand out one waiting Pokémon or item whose badges are met, skipping the first `skip` that
// can't be placed. Returns a CE_DELIVERY_ value and fills gStringVar1 (species or item name),
// gStringVar2 (box name) and gStringVar3 (quantity); a delivered Pokémon's box and slot are in
// gSpecialVar_MonBoxId and gSpecialVar_MonBoxPos, like CopyMonToPC leaves them.
u32 CeTeam_DeliverNext(u32 skip);
// Whether gCeTeamMons[i] / start.pc[i] was handed out.
bool32 CeTeam_MonDelivered(u32 i);
bool32 CeTeam_ItemDelivered(u32 i);

// Script natives (data/scripts/ce/team.inc and one-line hooks).
void CeTeam_ScriptDeliverNext(struct ScriptContext *ctx);   // PC: VAR_0x8006 = skip count in, VAR_RESULT out
void CeTeam_ScriptGiftGate(struct ScriptContext *ctx);      // gift NPCs: jump to the refusal if no new Pokémon
void CeTeam_ScriptNoRoomGate(struct ScriptContext *ctx);    // "no more room": say why instead, for a static team
void CeTeam_ScriptTradeGate(struct ScriptContext *ctx);     // ingame_trade: decline
void CeTeam_ScriptSafariGate(struct ScriptContext *ctx);    // Safari Zone counter: turn away
void CeTeam_ScriptStarterGate(struct ScriptContext *ctx);   // Birch's lab: skip "have this Pokémon"

#endif // GUARD_CE_TEAM_H
