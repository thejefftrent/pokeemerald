#include "global.h"
#include "battle.h"
#include "event_data.h"
#include "item.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "trainer_util.h"
#include "ce/rom_config.h"
#include "ce/team.h"
#include "ce/team_table.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/region_map_sections.h"

// Static team challenge and badge-timed PC deliveries. See include/ce/team.h.

STATIC_ASSERT(sizeof(struct CeTeamSave) == 16, CeTeamSaveSize);
STATIC_ASSERT(offsetof(struct SaveBlock2, pokedex.ceTeam) == 0x6C, CeTeamSaveOffset);
STATIC_ASSERT(CE_TEAM_MAX_MONS <= 32 * ARRAY_COUNT(((struct CeTeamSave *)0)->monsDelivered), CeTeamMonsFitSave);
STATIC_ASSERT(CE_START_PC_ITEMS <= 16, CeTeamItemsFitSave);
STATIC_ASSERT(TOTAL_BOXES_COUNT < CE_TEAM_IN_PARTY, CeTeamBoxIdsFit);

#define sSave (gSaveBlock2Ptr->pokedex.ceTeam)

// Mixed into each Pokémon's personality seed, so two runs by the same trainer differ only by index.
#define CE_TEAM_SEED 0xCE7EA000

static const u8 sText_CatchBlocked[] = _("Your static team can't take in\nnew POKéMON on this run!\p");

bool32 CeTeam_SaveIsSetUp(void)
{
    return sSave.magic == CE_TEAM_SAVE_MAGIC && sSave.version == CE_TEAM_SAVE_VERSION;
}

bool32 CeTeam_IsStaticTeam(void)
{
    return CeTeam_SaveIsSetUp() && sSave.staticTeam;
}

bool32 CeTeam_RefusesNewMons(void)
{
    return CeTeam_IsStaticTeam();
}

bool32 CeTeam_BlocksCatching(void)
{
    // Wally catches his Ralts himself, and trainer battles never allow a catch anyway.
    return CeTeam_IsStaticTeam() && !(gBattleTypeFlags & (BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_TRAINER));
}

const u8 *CeTeam_CatchBlockedText(void)
{
    return sText_CatchBlocked;
}

u32 CeTeam_BadgeCount(void)
{
    u32 i, count = 0;

    for (i = 0; i < CE_TEAM_MAX_BADGES; i++)
    {
        if (FlagGet(FLAG_BADGE01_GET + i))
            count++;
    }
    return count;
}

static u32 TeamCount(void)
{
    u32 i;

    for (i = 0; i < CE_TEAM_MAX_MONS && gCeTeamMons[i].mon.species != SPECIES_NONE; i++)
        ;
    return i;
}

static bool32 HasTimedPcItems(void)
{
    const struct CeStartConfig *start = &gCeRomConfig.body.start;
    u32 i;

    if (!start->replacePcItems)
        return FALSE;
    for (i = 0; i < CE_START_PC_ITEMS && start->pc[i].item != ITEM_NONE; i++)
    {
        if (start->pcBadges[i] != 0)
            return TRUE;
    }
    return FALSE;
}

bool32 CeTeam_MonDelivered(u32 i)
{
    return i < CE_TEAM_MAX_MONS && (sSave.monsDelivered[i / 32] & (1u << (i % 32)));
}

static void MarkMonDelivered(u32 i)
{
    sSave.monsDelivered[i / 32] |= 1u << (i % 32);
}

bool32 CeTeam_ItemDelivered(u32 i)
{
    return i < CE_START_PC_ITEMS && (sSave.itemsDelivered & (1u << i));
}

// The Pokémon gCeTeamMons[i] describes, with the player as its trainer: it obeys at any level
// and counts as the player's own. Personality comes from the player's trainer id and i.
static void CreateTeamMon(struct Pokemon *mon, u32 i)
{
    struct TrainerGenerator gen = {0};
    u32 value;

    gen.gender = gSaveBlock2Ptr->playerGender;
    gen.smartTera = TRUE; // keep the species' own Tera type unless one is set
    StringCopyN(gen.name, gSaveBlock2Ptr->playerName, PLAYER_NAME_LENGTH + 1);
    gen.otID = OTID_STRUCT_PLAYER_ID;
    gen.localRngState = LocalRandomSeed(READ_OTID_FROM_SAVE ^ (CE_TEAM_SEED + i));
    GenerateMonFromTrainerMon(mon, &gCeTeamMons[i].mon, &gen);
    // Mew and Deoxys only obey Pokémon from a fateful encounter, and a gift is one.
    value = TRUE;
    SetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER, &value);
    MonRestorePP(mon);
}

static void RegisterInPokedex(struct Pokemon *mon)
{
    HandleSetPokedexFlagFromMon(mon, FLAG_SET_SEEN);
    HandleSetPokedexFlagFromMon(mon, FLAG_SET_CAUGHT);
}

static bool32 SlotIsFree(u32 box, u32 slot)
{
    return GetBoxMonDataAt(box, slot, MON_DATA_SPECIES) == SPECIES_NONE;
}

// Put a team Pokémon into the PC: its own slot, or the first free one from box 1 on.
// Returns CE_DELIVERY_MON, _MON_MOVED or _MON_NO_ROOM.
static u32 PlaceInPc(struct Pokemon *mon, u32 box, u32 slot)
{
    u32 b, s;

    if (box < TOTAL_BOXES_COUNT && slot < IN_BOX_COUNT && SlotIsFree(box, slot))
    {
        SetBoxMonAt(box, slot, &mon->box);
        gSpecialVar_MonBoxId = box;
        gSpecialVar_MonBoxPos = slot;
        return CE_DELIVERY_MON;
    }
    for (b = 0; b < TOTAL_BOXES_COUNT; b++)
    {
        for (s = 0; s < IN_BOX_COUNT; s++)
        {
            if (SlotIsFree(b, s))
            {
                SetBoxMonAt(b, s, &mon->box);
                gSpecialVar_MonBoxId = b;
                gSpecialVar_MonBoxPos = s;
                return CE_DELIVERY_MON_MOVED;
            }
        }
    }
    return CE_DELIVERY_MON_NO_ROOM;
}

void CeTeam_OnNewGame(void)
{
    const struct CeTeamMon *team = gCeTeamMons;
    const struct CeStartConfig *start = &gCeRomConfig.body.start;
    u32 i, count = TeamCount(), party = 0;
    struct Pokemon mon;

    memset(&sSave, 0, sizeof(sSave));
    // A ROM without a team or timed PC items leaves nothing in the save, so the run stays vanilla.
    if (count == 0 && !gCeRomConfig.body.team.staticTeam && !HasTimedPcItems())
        return;
    sSave.magic = CE_TEAM_SAVE_MAGIC;
    sSave.version = CE_TEAM_SAVE_VERSION;
    sSave.staticTeam = gCeRomConfig.body.team.staticTeam;

    for (i = 0; i < count; i++)
    {
        u32 metLocation = MAPSEC_LITTLEROOT_TOWN;

        if (team[i].badges != 0 && team[i].box != CE_TEAM_IN_PARTY)
            continue;
        CreateTeamMon(&mon, i);
        SetMonData(&mon, MON_DATA_MET_LOCATION, &metLocation);
        if (team[i].box == CE_TEAM_IN_PARTY)
        {
            if (party >= PARTY_SIZE)
                continue;
            CopyMon(&gParties[B_TRAINER_PLAYER][party++], &mon, sizeof(mon));
        }
        else if (PlaceInPc(&mon, team[i].box, team[i].slot) == CE_DELIVERY_MON_NO_ROOM)
        {
            continue;
        }
        RegisterInPokedex(&mon);
        MarkMonDelivered(i);
    }
    CalculatePlayerPartyCount();

    // PC items that need no badge are already in the PC (CeStart_OnNewGame).
    for (i = 0; i < CE_START_PC_ITEMS && start->pc[i].item != ITEM_NONE; i++)
    {
        if (!start->replacePcItems || start->pcBadges[i] == 0)
            sSave.itemsDelivered |= 1u << i;
    }
}

static u32 DeliverMon(u32 i)
{
    struct Pokemon mon;
    u32 result;

    CreateTeamMon(&mon, i);
    StringCopy(gStringVar1, GetSpeciesName(gCeTeamMons[i].mon.species));
    result = PlaceInPc(&mon, gCeTeamMons[i].box, gCeTeamMons[i].slot);
    if (result == CE_DELIVERY_MON_NO_ROOM)
        return result;
    StringCopy(gStringVar2, GetBoxNamePtr(gSpecialVar_MonBoxId));
    RegisterInPokedex(&mon);
    MarkMonDelivered(i);
    return result;
}

static u32 DeliverItem(u32 i)
{
    const struct CeStartItem *entry = &gCeRomConfig.body.start.pc[i];
    u32 quantity = min(entry->quantity, MAX_PC_ITEM_CAPACITY);

    CopyItemNameHandlePlural(entry->item, gStringVar1, quantity);
    ConvertIntToDecimalStringN(gStringVar3, quantity, STR_CONV_MODE_LEFT_ALIGN, 3);
    if (!AddPCItem(entry->item, quantity))
        return CE_DELIVERY_ITEM_NO_ROOM;
    sSave.itemsDelivered |= 1u << i;
    return CE_DELIVERY_ITEM;
}

u32 CeTeam_DeliverNext(u32 skip)
{
    const struct CeStartConfig *start = &gCeRomConfig.body.start;
    u32 i, result, badges, count, skipped = 0;

    if (!CeTeam_SaveIsSetUp())
        return CE_DELIVERY_NONE;
    badges = CeTeam_BadgeCount();
    count = TeamCount();
    for (i = 0; i < count; i++)
    {
        const struct CeTeamMon *entry = &gCeTeamMons[i];
        if (entry->box == CE_TEAM_IN_PARTY || CeTeam_MonDelivered(i) || entry->badges > badges)
            continue;
        result = DeliverMon(i);
        if (result != CE_DELIVERY_MON_NO_ROOM || skipped++ == skip)
            return result;
    }
    if (!start->replacePcItems)
        return CE_DELIVERY_NONE;
    for (i = 0; i < CE_START_PC_ITEMS && start->pc[i].item != ITEM_NONE; i++)
    {
        if (CeTeam_ItemDelivered(i) || start->pcBadges[i] > badges || start->pc[i].item >= ITEMS_COUNT || start->pc[i].quantity == 0)
            continue;
        result = DeliverItem(i);
        if (result != CE_DELIVERY_ITEM_NO_ROOM || skipped++ == skip)
            return result;
    }
    return CE_DELIVERY_NONE;
}

// PC deliveries, one per call: the script says what arrived and calls again, adding 1 to
// VAR_0x8006 after each "no room" so that one is skipped next time.
void CeTeam_ScriptDeliverNext(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    gSpecialVar_Result = CeTeam_DeliverNext(gSpecialVar_0x8006);
}

extern const u8 CeEventScript_GiftRefused[];
extern const u8 CeEventScript_NoRoomStaticTeam[];
extern const u8 CeEventScript_TradeRefused[];
extern const u8 CeEventScript_SafariRefused[];
extern const u8 CeEventScript_StarterKept[];

void CeTeam_ScriptGiftGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeTeam_RefusesNewMons())
        ScriptJump(ctx, CeEventScript_GiftRefused);
}

void CeTeam_ScriptNoRoomGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeTeam_RefusesNewMons())
        ScriptJump(ctx, CeEventScript_NoRoomStaticTeam);
}

void CeTeam_ScriptTradeGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeTeam_RefusesNewMons())
        ScriptJump(ctx, CeEventScript_TradeRefused);
}

void CeTeam_ScriptSafariGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeTeam_RefusesNewMons())
        ScriptJump(ctx, CeEventScript_SafariRefused);
}

void CeTeam_ScriptStarterGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeTeam_IsStaticTeam())
        ScriptJump(ctx, CeEventScript_StarterKept);
}
