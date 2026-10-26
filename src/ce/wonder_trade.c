#include "global.h"
#include "event_data.h"
#include "item.h"
#include "mail.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "string_util.h"
#include "trade.h"
#include "ce/field_moves.h"
#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/team.h"
#include "ce/third_type.h"
#include "ce/wonder_trade.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"

// Wonder Trade. See include/ce/wonder_trade.h and docs/wonder-trade.md.

// Forms that only exist in battle (or can't be obtained) but carry no flag in gSpeciesInfo: the
// same list as the engine's BATTLE_ONLY_FORMS (packages/engine/src/randomizer/species-info.ts).
// The self-test prints its count and id sum and the canary compares them with the engine's, so
// the two lists can't drift. Mega, Primal, Ultra Burst, Gigantamax, Tera and totem forms are
// recognised by their flags.
static const u16 sBattleOnlyForms[] =
{
    SPECIES_CASTFORM_SUNNY,
    SPECIES_CASTFORM_RAINY,
    SPECIES_CASTFORM_SNOWY,
    SPECIES_CHERRIM_SUNSHINE,
    SPECIES_DARMANITAN_ZEN,
    SPECIES_DARMANITAN_GALAR_ZEN,
    SPECIES_MELOETTA_PIROUETTE,
    SPECIES_GRENINJA_ASH,
    SPECIES_AEGISLASH_BLADE,
    SPECIES_XERNEAS_ACTIVE,
    SPECIES_ZYGARDE_COMPLETE,
    SPECIES_WISHIWASHI_SCHOOL,
    SPECIES_MINIOR_CORE_RED,
    SPECIES_MINIOR_CORE_ORANGE,
    SPECIES_MINIOR_CORE_YELLOW,
    SPECIES_MINIOR_CORE_GREEN,
    SPECIES_MINIOR_CORE_BLUE,
    SPECIES_MINIOR_CORE_INDIGO,
    SPECIES_MINIOR_CORE_VIOLET,
    SPECIES_MIMIKYU_BUSTED,
    SPECIES_NECROZMA_ULTRA,
    SPECIES_CRAMORANT_GULPING,
    SPECIES_CRAMORANT_GORGING,
    SPECIES_EISCUE_NOICE,
    SPECIES_MORPEKO_HANGRY,
    SPECIES_ZACIAN_CROWNED,
    SPECIES_ZAMAZENTA_CROWNED,
    SPECIES_ETERNATUS_ETERNAMAX,
    SPECIES_PALAFIN_HERO,
    SPECIES_TERAPAGOS_TERASTAL,
    SPECIES_TERAPAGOS_STELLAR,
    // Let's Go partners: can't evolve and only exist as a gift in those games.
    SPECIES_PIKACHU_STARTER,
    SPECIES_EEVEE_STARTER,
};

// The other trainers. An OT name is at most PLAYER_NAME_LENGTH characters.
static const struct
{
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 gender;
} sOtNames[] =
{
    { _("KENJI"),  MALE },
    { _("MARCO"),  MALE },
    { _("OSCAR"),  MALE },
    { _("TOMAS"),  MALE },
    { _("RAFAEL"), MALE },
    { _("HUGO"),   MALE },
    { _("YUSUF"),  MALE },
    { _("ARJUN"),  MALE },
    { _("AIKO"),   FEMALE },
    { _("NADIA"),  FEMALE },
    { _("PRIYA"),  FEMALE },
    { _("LENA"),   FEMALE },
    { _("SOFIA"),  FEMALE },
    { _("INES"),   FEMALE },
    { _("CLARA"),  FEMALE },
    { _("MEI"),    FEMALE },
};

u32 CeWonderTrade_BattleOnlyCount(void)
{
    return ARRAY_COUNT(sBattleOnlyForms);
}

u16 CeWonderTrade_BattleOnlyForm(u32 i)
{
    return i < ARRAY_COUNT(sBattleOnlyForms) ? sBattleOnlyForms[i] : SPECIES_NONE;
}

static bool32 IsBattleOnlyForm(u16 species)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sBattleOnlyForms); i++)
    {
        if (sBattleOnlyForms[i] == species)
            return TRUE;
    }
    return FALSE;
}

u16 CeWonderTrade_GroupOf(u16 species)
{
    const u16 *forms;

    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return SPECIES_NONE;
    forms = gSpeciesInfo[species].formSpeciesIdTable;
    return forms != NULL ? forms[0] : species;
}

bool32 CeWonderTrade_InPool(u16 species, u32 type)
{
    const struct SpeciesInfo *info;

    if (species == SPECIES_NONE || species >= NUM_SPECIES || !IsSpeciesEnabled(species))
        return FALSE;
    info = &gSpeciesInfo[species];
    if (info->natDexNum == 0 || info->cannotBeTraded)
        return FALSE;
    // A base form (the first of its form table) or a regional form.
    if (CeWonderTrade_GroupOf(species) != species
     && !(info->isAlolanForm || info->isGalarianForm || info->isHisuianForm || info->isPaldeanForm))
        return FALSE;
    if (info->isMegaEvolution || info->isPrimalReversion || info->isUltraBurst || info->isGigantamax
     || info->isTeraForm || info->isTotem || IsBattleOnlyForm(species))
        return FALSE;
    return type == TYPE_NONE || CeSpecies_IsOfType(species, type);
}

// The species of a group: its head (a species that is the first of its form table, or has none)
// and the rest of the table. Returns how many of them are in the pool and, if `pick` is below
// that, the pick-th of them in *picked.
static u32 GroupMembersInPool(u16 head, u32 type, u32 pick, u16 *picked)
{
    const u16 *forms = gSpeciesInfo[head].formSpeciesIdTable;
    u32 i, count = 0;

    if (forms == NULL)
    {
        if (!CeWonderTrade_InPool(head, type))
            return 0;
        if (pick == 0 && picked != NULL)
            *picked = head;
        return 1;
    }
    for (i = 0; forms[i] != FORM_SPECIES_END; i++)
    {
        if (CeWonderTrade_InPool(forms[i], type))
        {
            if (count == pick && picked != NULL)
                *picked = forms[i];
            count++;
        }
    }
    return count;
}

static bool32 IsGroupHead(u16 species)
{
    return CeWonderTrade_GroupOf(species) == species;
}

u32 CeWonderTrade_CountGroups(u32 type)
{
    u32 species, count = 0;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (IsGroupHead(species) && GroupMembersInPool(species, type, 0, NULL) != 0)
            count++;
    }
    return count;
}

u16 CeWonderTrade_PickSpecies(u32 type)
{
    u32 groups = CeWonderTrade_CountGroups(type);
    u32 species, members, k;
    u16 picked = SPECIES_NONE;

    if (groups == 0)
        return SPECIES_NONE;
    k = Random32() % groups;
    for (species = 1; species < NUM_SPECIES; species++)
    {
        if (!IsGroupHead(species))
            continue;
        members = GroupMembersInPool(species, type, 0, NULL);
        if (members == 0)
            continue;
        if (k-- == 0)
        {
            GroupMembersInPool(species, type, Random() % members, &picked);
            break;
        }
    }
    return picked;
}

bool32 CeWonderTrade_IsAllowed(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    return !CeTeam_RefusesNewMons()
        && !rules->nuzlocke
        && rules->onlySpecies == SPECIES_NONE
        && rules->partyLimit == 0;
}

u32 CeWonderTrade_Block(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    if (!CeQol_IsOn(CE_QOL_WONDER_TRADE))
        return CE_WT_REFUSED_OFF;
    if (CeTeam_RefusesNewMons())
        return CE_WT_REFUSED_STATIC_TEAM;
    if (rules->nuzlocke || rules->onlySpecies != SPECIES_NONE || rules->partyLimit != 0)
        return CE_WT_REFUSED_RULES;
    if (CeWonderTrade_CountGroups(rules->monotype) == 0)
        return CE_WT_REFUSED_EMPTY_POOL;
    return CE_WT_OPEN;
}

void CeWonderTrade_CreateMon(struct Pokemon *mon, u16 species, u8 level)
{
    u32 otId = Random32();
    u32 personality = Random32();
    u32 ot = Random() % ARRAY_COUNT(sOtNames);
    metloc_u8_t metLocation = METLOC_IN_GAME_TRADE;
    u8 otGender = sOtNames[ot].gender;

    // Never the player's own id: the Pokémon must count as traded (IsTradedMon).
    if (otId == READ_OTID_FROM_SAVE)
        otId ^= 1;
    CreateMonWithIVs(mon, species, level, personality, OTID_STRUCT_PRESET(otId), USE_RANDOM_IVS);
    GiveMonInitialMoveset(mon);
    SetMonData(mon, MON_DATA_OT_NAME, sOtNames[ot].name);
    SetMonData(mon, MON_DATA_OT_GENDER, &otGender);
    SetMonData(mon, MON_DATA_MET_LOCATION, &metLocation);
    CalculateMonStats(mon);
}

static bool32 IsPcPick(void)
{
    return gSpecialVar_0x8004 == PC_MON_CHOSEN;
}

// Whether a Pokémon other than the chosen one knows Surf: the party's (Eggs left out) and every
// PC box's.
static bool32 OtherMonKnowsSurf(void)
{
    u32 i, box, pos;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (!IsPcPick() && i == gSpecialVar_0x8004)
            continue;
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG)
         && MonKnowsMove(mon, MOVE_SURF))
            return TRUE;
    }
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(box, pos);

            if (IsPcPick() && box == gSpecialVar_MonBoxId && pos == gSpecialVar_MonBoxPos)
                continue;
            if (GetBoxMonData(boxMon, MON_DATA_SPECIES) != SPECIES_NONE && !GetBoxMonData(boxMon, MON_DATA_IS_EGG)
             && BoxMonKnowsMove(boxMon, MOVE_SURF))
                return TRUE;
        }
    }
    return FALSE;
}

// Whether the party keeps a Pokémon that can battle without the chosen one.
static bool32 PartyKeepsAMon(void)
{
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];

        if (i != gSpecialVar_0x8004 && GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(mon, MON_DATA_IS_EGG) && GetMonData(mon, MON_DATA_HP) != 0)
            return TRUE;
    }
    return FALSE;
}

u32 CeWonderTrade_CheckMon(void)
{
    struct BoxPokemon *boxMon = GetSelectedBoxMonFromPcOrParty();
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    u16 item = GetBoxMonData(boxMon, MON_DATA_HELD_ITEM);

    GetBoxMonData(boxMon, MON_DATA_NICKNAME, nickname);
    StringCopy_Nickname(gStringVar1, nickname);
    if (item != ITEM_NONE)
        CopyItemName(item, gStringVar2);
    else
        gStringVar2[0] = EOS;

    if (GetBoxMonData(boxMon, MON_DATA_IS_EGG))
        return CE_WT_EGG;
    if (CeModes_IsDeadBox(boxMon))
        return CE_WT_DEAD;
    if (gSpeciesInfo[SanitizeSpeciesId(GetBoxMonData(boxMon, MON_DATA_SPECIES))].cannotBeTraded)
        return CE_WT_CANT_TRADE;
    if (item != ITEM_NONE && ItemIsMail(item))
        return CE_WT_MAIL;
    if (item != ITEM_NONE && !CheckBagHasSpace(item, 1))
        return CE_WT_BAG_FULL;
    if (!IsPcPick() && !PartyKeepsAMon())
        return CE_WT_LAST_MON;
    if (BoxMonKnowsMove(boxMon, MOVE_SURF) && !OtherMonKnowsSurf() && !CeFieldMoves_CanSurfWithoutMove())
        return CE_WT_LAST_SURFER;
    return CE_WT_OK;
}

static struct Pokemon *ReceivedMon(void)
{
    if (IsPcPick())
        return &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC];
    return &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
}

bool32 CeWonderTrade_FixPcEvolution(void)
{
    struct Pokemon *received = &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC];
    struct BoxPokemon *boxMon;

    if (!IsPcPick() || gSpecialVar_MonBoxId >= TOTAL_BOXES_COUNT || gSpecialVar_MonBoxPos >= IN_BOX_COUNT)
        return FALSE;
    // CopyMonToPC left the received Pokémon's box and slot here (maybe not the sent one's).
    boxMon = GetBoxedMonPtr(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos);
    if (GetBoxMonData(boxMon, MON_DATA_SPECIES) == SPECIES_NONE
     || GetBoxMonData(boxMon, MON_DATA_PERSONALITY) != GetMonData(received, MON_DATA_PERSONALITY)
     || GetBoxMonData(boxMon, MON_DATA_SPECIES) == GetMonData(received, MON_DATA_SPECIES))
        return FALSE;
    *boxMon = received->box;
    return TRUE;
}

bool32 CeWonderTrade_BufferSceneStrings(void)
{
    u8 name[POKEMON_NAME_BUFFER_SIZE];

    if (!FlagGet(FLAG_CE_WONDER_TRADE))
        return FALSE;
    // Before the swap the received Pokémon is the opponent's first, and the sent one is where
    // ReceivedMon will be (a PC pick was moved out of its box when the scene started).
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_OT_NAME, gStringVar1);
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar3, name);
    GetMonData(ReceivedMon(), MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar2, name);
    return TRUE;
}

bool32 CeWonderTrade_BufferMonName(void)
{
    u8 name[POKEMON_NAME_BUFFER_SIZE];

    if (!FlagGet(FLAG_CE_WONDER_TRADE))
        return FALSE;
    // After the swap: the received Pokémon is where the sent one was.
    GetMonData(ReceivedMon(), MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar1, name);
    StringCopy(gStringVar2, GetSpeciesName(GetMonData(ReceivedMon(), MON_DATA_SPECIES)));
    return TRUE;
}

void CeWonderTrade_ScriptGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    FlagClear(FLAG_CE_WONDER_TRADE);
    gSpecialVar_Result = CeWonderTrade_Block();
}

void CeWonderTrade_ScriptCheckChosen(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    gSpecialVar_Result = CeWonderTrade_CheckMon();
}

void CeWonderTrade_ScriptPrepare(struct ScriptContext *ctx)
{
    struct BoxPokemon *boxMon = GetSelectedBoxMonFromPcOrParty();
    u16 species = CeWonderTrade_PickSpecies(CeModes_Rules()->monotype);
    u16 item = GetBoxMonData(boxMon, MON_DATA_HELD_ITEM);
    u16 none = ITEM_NONE;

    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    gSpecialVar_Result = 0;
    if (species == SPECIES_NONE)
        return;
    if (item != ITEM_NONE)
    {
        if (ItemIsMail(item) || !AddBagItem(item, 1))
            return;
        SetBoxMonData(boxMon, MON_DATA_HELD_ITEM, &none);
        CopyItemName(item, gStringVar2);
    }
    CeWonderTrade_CreateMon(&gParties[B_TRAINER_OPPONENT_A][0], species, GetLevelFromBoxMonExp(boxMon));
    FlagSet(FLAG_CE_WONDER_TRADE);
    gSpecialVar_Result = item != ITEM_NONE ? 2 : 1;
}

void CeWonderTrade_ScriptFinish(struct ScriptContext *ctx)
{
    struct Pokemon *received = ReceivedMon();
    u8 name[POKEMON_NAME_BUFFER_SIZE];

    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    CeWonderTrade_FixPcEvolution();
    GetMonData(received, MON_DATA_NICKNAME, name);
    StringCopy_Nickname(gStringVar1, name);
    GetMonData(received, MON_DATA_OT_NAME, gStringVar2);
    if (IsPcPick())
        StringCopy(gStringVar3, GetBoxNamePtr(gSpecialVar_MonBoxId));
    gSpecialVar_Result = IsPcPick();
    FlagClear(FLAG_CE_WONDER_TRADE);
}
