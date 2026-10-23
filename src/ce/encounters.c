#include "global.h"
#include "event_data.h"
#include "mail.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "trade.h"
#include "constants/items.h"
#include "ce/encounters.h"

// Game-side reader of gCeEncounters (defined in encounters_data.c, written by the website).

STATIC_ASSERT(CE_ENC_STARTER_FIRE == CE_ENC_STARTER_GRASS + 1 && CE_ENC_STARTER_WATER == CE_ENC_STARTER_GRASS + 2, ceStartersInSlotOrder);

#define CE_STARTER_SLOTS 3

static const struct CeEncounter *Get(enum CeEncounterId id)
{
    return &gCeEncounters[id < CE_ENC_COUNT ? id : 0];
}

u16 CeEnc_Species(enum CeEncounterId id)
{
    return Get(id)->species;
}

u8 CeEnc_Level(enum CeEncounterId id)
{
    return Get(id)->level;
}

u16 CeEnc_Item(enum CeEncounterId id)
{
    return Get(id)->item;
}

static bool32 HasFlag(enum CeEncounterId id, enum CeEncounterFlag flag)
{
    return (Get(id)->flags & flag) != 0;
}

static void ApplyFlagsToBoxMon(struct BoxPokemon *boxMon, enum CeEncounterId id)
{
    if (HasFlag(id, CE_ENC_FLAG_SHINY))
    {
        u8 shiny = TRUE;
        SetBoxMonData(boxMon, MON_DATA_IS_SHINY, &shiny);
    }
}

void CeEnc_ApplyFlags(struct Pokemon *mon, enum CeEncounterId id)
{
    ApplyFlagsToBoxMon(&mon->box, id);
}

void CeEnc_LoadVars(enum CeEncounterId id)
{
    const struct CeEncounter *enc = Get(id);
    VarSet(VAR_CE_ENC_SPECIES, enc->species);
    VarSet(VAR_CE_ENC_LEVEL, enc->level);
    VarSet(VAR_CE_ENC_ITEM, enc->item);
}

// giveResult is what givemon / ScriptGiveMon returned: the mon is the last one in the party,
// or the box slot CopyMonToPC reported.
void CeEnc_ApplyFlagsToGiven(enum CeEncounterId id, u32 giveResult)
{
    u32 count;

    if (giveResult == MON_GIVEN_TO_PARTY)
    {
        count = CalculatePlayerPartyCount();
        if (count > 0)
            ApplyFlagsToBoxMon(&gParties[B_TRAINER_PLAYER][count - 1].box, id);
    }
    else if (giveResult == MON_GIVEN_TO_PC)
    {
        ApplyFlagsToBoxMon(GetBoxedMonPtr(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos), id);
    }
}

void CeEnc_CreateWildMon(enum CeEncounterId id)
{
    const struct CeEncounter *enc = Get(id);
    CreateScriptedWildMon(enc->species, enc->level, enc->item);
    CeEnc_ApplyFlags(&gParties[B_TRAINER_OPPONENT_A][0], id);
}

// Same as the seteventmon macro (special CreateEnemyEventMon reads 0x8004-0x8006).
void CeEnc_CreateEventMon(enum CeEncounterId id)
{
    const struct CeEncounter *enc = Get(id);
    gSpecialVar_0x8004 = enc->species;
    gSpecialVar_0x8005 = enc->level;
    gSpecialVar_0x8006 = enc->item;
    CreateEnemyEventMon();
    CeEnc_ApplyFlags(&gParties[B_TRAINER_OPPONENT_A][0], id);
}

// ---- Script natives (asm/macros/ce_encounters.inc) ----

// ce_loadenc id
void CeScript_LoadEncounter(struct ScriptContext *ctx)
{
    enum CeEncounterId id = ScriptReadHalfword(ctx);

    Script_RequestEffects(SCREFF_V1);
    CeEnc_LoadVars(id);
}

// The last step of ce_givemon id, after givemon.
void CeScript_ApplyGiftFlags(struct ScriptContext *ctx)
{
    enum CeEncounterId id = ScriptReadHalfword(ctx);

    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    CeEnc_ApplyFlagsToGiven(id, gSpecialVar_Result);
}

// setwildbattle_ce id
void CeScript_SetWildBattle(struct ScriptContext *ctx)
{
    enum CeEncounterId id = ScriptReadHalfword(ctx);

    Script_RequestEffects(SCREFF_V1);
    CeEnc_CreateWildMon(id);
}

// seteventmon_ce id
void CeScript_SetEventMon(struct ScriptContext *ctx)
{
    enum CeEncounterId id = ScriptReadHalfword(ctx);

    Script_RequestEffects(SCREFF_V1);
    CeEnc_CreateEventMon(id);
}

// ---- Starters ----

u16 CeEnc_Starter(u32 slot)
{
    return CeEnc_Species(CE_ENC_STARTER_GRASS + (slot < CE_STARTER_SLOTS ? slot : 0));
}

void CeEnc_GiveStarter(u16 species, u32 slot)
{
    enum CeEncounterId id = CE_ENC_STARTER_GRASS + (slot < CE_STARTER_SLOTS ? slot : 0);
    CeEnc_ApplyFlagsToGiven(id, ScriptGiveMon(species, CeEnc_Level(id), CeEnc_Item(id)));
}

// ---- In-game trades ----

// The table row for one INGAME_TRADE_* entry, or CE_ENC_COUNT if the trade isn't in the table.
static enum CeEncounterId FindTradeRow(u32 trade, enum CeEncounterKind kind)
{
    enum CeEncounterId id;

    for (id = 0; id < CE_ENC_COUNT; id++)
    {
        if (gCeEncounterInfo[id].kind == kind && gCeEncounterInfo[id].trade == trade)
            break;
    }
    return id;
}

u16 CeEnc_TradeSpecies(u32 trade, u16 vanillaSpecies)
{
    enum CeEncounterId id = FindTradeRow(trade, CE_ENC_KIND_TRADE);
    return id < CE_ENC_COUNT ? CeEnc_Species(id) : vanillaSpecies;
}

u16 CeEnc_TradeRequest(u32 trade, u16 vanillaRequest)
{
    enum CeEncounterId id = FindTradeRow(trade, CE_ENC_KIND_TRADE_REQUEST);
    return id < CE_ENC_COUNT ? CeEnc_Species(id) : vanillaRequest;
}

// A replaced species doesn't keep the original mon's nickname.
const u8 *CeEnc_TradeNickname(u32 trade, u16 vanillaSpecies, const u8 *vanillaNickname)
{
    u16 species = CeEnc_TradeSpecies(trade, vanillaSpecies);
    return species == vanillaSpecies ? vanillaNickname : GetSpeciesName(species);
}

// After CreateInGameTradePokemonInternal has given the mon its vanilla held item and mail:
// swap in the table's item (the trade's mail only goes with its vanilla mail item) and flags.
void CeEnc_FinishTradeMon(struct Pokemon *mon, u32 trade, u16 vanillaItem)
{
    enum CeEncounterId id = FindTradeRow(trade, CE_ENC_KIND_TRADE);
    u16 item;

    if (id >= CE_ENC_COUNT)
        return;
    item = CeEnc_Item(id);
    if (item != vanillaItem)
    {
        u8 mail = MAIL_NONE;
        if (ItemIsMail(item))
            item = ITEM_NONE;
        SetMonData(mon, MON_DATA_HELD_ITEM, &item);
        SetMonData(mon, MON_DATA_MAIL, &mail);
    }
    else if (item != ITEM_NONE && ItemIsMail(item))
    {
        gTradeMail[0].species = CeEnc_Species(id);
    }
    CeEnc_ApplyFlags(mon, id);
}
