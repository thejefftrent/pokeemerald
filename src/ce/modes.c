#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_script_commands.h"
#include "event_data.h"
#include "item.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "string_util.h"
#include "ce/modes.h"
#include "ce/rom_config.h"
#include "ce/team.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/layouts.h"
#include "constants/region_map_sections.h"

// Rules come from the save's copy (CeModes_Rules), never straight from gCeRomConfig, so
// a re-patched ROM can't change a run in progress. See include/ce/modes.h.

STATIC_ASSERT(sizeof(struct CeModesConfig) == 16, CeModesConfigSize);
STATIC_ASSERT(sizeof(struct CeModesSave) == 52, CeModesSaveSize);
STATIC_ASSERT(offsetof(struct SaveBlock2, pokedex.ceModes) == 0x38, CeModesSaveOffset);
STATIC_ASSERT(MAPSEC_COUNT <= 32 * CE_MODES_AREA_WORDS, CeModesAreasFitSave);
STATIC_ASSERT(MAPSEC_COUNT < CE_AREA_FREE, CeModesAreaIdsFitTable);

// Battles no rule applies to. Battle Tents keep their own copy of the party, like the Frontier.
#define CE_EXEMPT_BATTLE_TYPES (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED | BATTLE_TYPE_RECORDED_LINK  \
                              | BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_CATCH_TUTORIAL | BATTLE_TYPE_SAFARI \
                              | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL \
                              | BATTLE_TYPE_SECRET_BASE | BATTLE_TYPE_TOWER_LINK_MULTI | BATTLE_TYPE_POKEDUDE)

#define sSave (gSaveBlock2Ptr->pokedex.ceModes)

static const struct CeModesConfig sNoRules = {0};

static const u8 sText_AreaUsed[] = _("You already had your encounter\nin this area!\p");
static const u8 sText_Dupe[] = _("You already own a POKéMON of\nthis evolution family!\p");
static const u8 sText_Monotype[] = _("Only {STR_VAR_1}-type POKéMON\ncan be caught on this run!\p");
static const u8 sText_Species[] = _("Only the {STR_VAR_1} family\ncan be caught on this run!\p");
static const u8 sText_NoItems[] = _("Items can't be used in battle\non this run!\p");
static const u8 sText_NoLevels[] = _("POKéMON can't gain levels\non this run!{PAUSE_UNTIL_PRESS}");

extern const u8 CeEventScript_CenterHealRefused[];
extern const u8 CeEventScript_RestHealRefused[];
extern const u8 CeEventScript_DaycareRefused[];

bool32 CeModes_SaveHasRules(void)
{
    return sSave.magic == CE_MODES_SAVE_MAGIC && sSave.version == CE_MODES_SAVE_VERSION;
}

const struct CeModesConfig *CeModes_Rules(void)
{
    return CeModes_SaveHasRules() ? &sSave.rules : &sNoRules;
}

void CeModes_OnNewGame(void)
{
    const struct CeModesConfig *rom = &gCeRomConfig.body.modes;

    memset(&sSave, 0, sizeof(sSave));
    // A ROM without rules leaves none in the save, so the run stays vanilla for good.
    if (memcmp(rom, &sNoRules, sizeof(sNoRules)) == 0)
        return;
    sSave.magic = CE_MODES_SAVE_MAGIC;
    sSave.version = CE_MODES_SAVE_VERSION;
    sSave.configHash = gCeRomConfig.hdr.configHash;
    sSave.rules = *rom;
}

enum CeBattleKind CeModes_BattleKind(void)
{
    if ((gBattleTypeFlags & CE_EXEMPT_BATTLE_TYPES) || gMapHeader.mapLayoutId == LAYOUT_BATTLE_TENT_BATTLE_ROOM)
        return CE_BATTLE_EXEMPT;
    if (gBattleTypeFlags & BATTLE_TYPE_TRAINER)
        return CE_BATTLE_TRAINER;
    if ((gBattleTypeFlags & (BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_ROAMER)) || FlagGet(FLAG_CE_SCRIPTED_WILD_BATTLE))
        return CE_BATTLE_SCRIPTED;
    return CE_BATTLE_WILD;
}

static bool32 HasType(u16 species, u32 type)
{
    return CeSpecies_IsOfType(species, type); // CE: monotype counts the third type
}

// The gates that decide which species the player may own at all.
static enum CeCatchBlock SpeciesRule(const struct CeModesConfig *rules, u16 species)
{
    if (rules->monotype != TYPE_NONE && !HasType(species, rules->monotype))
        return CE_CATCH_MONOTYPE;
    if (rules->onlySpecies != SPECIES_NONE && CeModes_FamilyRoot(species) != CeModes_FamilyRoot(rules->onlySpecies))
        return CE_CATCH_SPECIES;
    return CE_CATCH_OK;
}

enum CeCatchBlock CeModes_CatchRule(u16 species, bool32 shiny, u32 area, enum CeBattleKind kind)
{
    const struct CeModesConfig *rules = CeModes_Rules();
    enum CeCatchBlock block;

    if (kind == CE_BATTLE_EXEMPT || kind == CE_BATTLE_TRAINER)
        return CE_CATCH_OK;
    block = SpeciesRule(rules, species);
    if (block != CE_CATCH_OK)
        return block;
    // The Nuzlocke area rule covers random encounters only.
    if (!rules->nuzlocke || kind != CE_BATTLE_WILD || area == CE_AREA_NONE)
        return CE_CATCH_OK;
    if (rules->shinyClause && shiny)
        return CE_CATCH_OK;
    if (CeModes_IsAreaUsed(area))
        return CE_CATCH_AREA_USED;
    if (rules->dupesClause && CeModes_FamilyOwned(species))
        return CE_CATCH_DUPE;
    return CE_CATCH_OK;
}

bool32 CeModes_CountsAsEncounter(u16 species, bool32 shiny)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    // Encounters the player could never have caught don't use up the area.
    if (rules->shinyClause && shiny)
        return FALSE;
    if (SpeciesRule(rules, species) != CE_CATCH_OK)
        return FALSE;
    if (rules->dupesClause && CeModes_FamilyOwned(species))
        return FALSE;
    return TRUE;
}

enum CeCatchBlock CeModes_CurrentCatchBlock(void)
{
    struct Pokemon *mon;

    if (CeTeam_BlocksCatching())
        return CE_CATCH_STATIC_TEAM;
    if (!CeModes_SaveHasRules())
        return CE_CATCH_OK;
    mon = GetBattlerMon(GetCatchingBattler());
    return CeModes_CatchRule(GetMonData(mon, MON_DATA_SPECIES), IsMonShiny(mon), CeModes_CurrentArea(), CeModes_BattleKind());
}

const u8 *CeModes_CatchBlockedText(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    switch (CeModes_CurrentCatchBlock())
    {
    case CE_CATCH_AREA_USED:
        return sText_AreaUsed;
    case CE_CATCH_DUPE:
        return sText_Dupe;
    case CE_CATCH_MONOTYPE:
        StringCopy(gStringVar1, gTypesInfo[rules->monotype].name);
        return sText_Monotype;
    case CE_CATCH_SPECIES:
        StringCopy(gStringVar1, GetSpeciesName(CeModes_FamilyRoot(rules->onlySpecies)));
        return sText_Species;
    case CE_CATCH_STATIC_TEAM:
        return CeTeam_CatchBlockedText();
    default:
        return NULL;
    }
}

bool32 CeModes_ForcesNickname(void)
{
    return CeModes_Rules()->forceNicknames;
}

bool32 CeModes_NoBagInTrainerBattles(void)
{
    return CeModes_Rules()->noBattleItems && CeModes_BattleKind() == CE_BATTLE_TRAINER;
}

bool32 CeModes_BlocksBattleItem(u16 item)
{
    enum CeBattleKind kind;

    if (!CeModes_Rules()->noBattleItems)
        return FALSE;
    kind = CeModes_BattleKind();
    if (kind == CE_BATTLE_EXEMPT)
        return FALSE;
    return kind == CE_BATTLE_TRAINER || GetItemBattleUsage(item) != EFFECT_ITEM_THROW_BALL;
}

const u8 *CeModes_NoItemsText(void)
{
    return sText_NoItems;
}

bool32 CeModes_NoEvolution(void)
{
    return CeModes_Rules()->noEvolution;
}

static enum CeNoExp NoExp(void)
{
    return CeModes_Rules()->noExp;
}

// Every battle, exempt ones included: secret bases and the first battle give Exp. too.
bool32 CeModes_NoBattleExp(void)
{
    return NoExp() >= CE_NO_EXP_BATTLE;
}

// Every item that uses ItemUseOutOfBattle_RareCandy: Rare Candy, Exp. Candies, the Candy Jar.
bool32 CeModes_NoLevelItems(void)
{
    return NoExp() >= CE_NO_EXP_ALL;
}

const u8 *CeModes_NoLevelItemsText(void)
{
    return sText_NoLevels;
}

bool32 CeModes_DaycareClosed(void)
{
    return NoExp() >= CE_NO_EXP_ALL;
}

void CeModes_ScriptDaycareGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (CeModes_DaycareClosed())
        ScriptJump(ctx, CeEventScript_DaycareRefused);
}

bool32 CeModes_ForcesSetStyle(void)
{
    return CeModes_Rules()->forceSetStyle;
}

u32 CeModes_PartyLimit(void)
{
    u32 limit = CeModes_Rules()->partyLimit;
    return (limit == 0 || limit > PARTY_SIZE) ? PARTY_SIZE : limit;
}

// For code that put a Pokémon straight into the party (the Day Care): extras go to the PC.
void CeModes_EnforcePartyLimit(void)
{
    u32 i, limit = CeModes_PartyLimit();

    if (limit >= PARTY_SIZE)
        return;
    CompactPartySlots();
    for (i = limit; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SANITY_HAS_SPECIES) && CopyMonToPC(mon) == MON_GIVEN_TO_PC)
            ZeroMonData(mon);
    }
    CompactPartySlots();
    CalculatePlayerPartyCount();
}

bool32 CeModes_CanHeal(enum CeHealSource source)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    switch (source)
    {
    case CE_HEAL_CENTER:
        return !rules->noCenterHeal;
    case CE_HEAL_REST:
        return !rules->noRestHeal;
    }
    return TRUE;
}

void CeModes_ScriptCenterHealGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (!CeModes_CanHeal(CE_HEAL_CENTER))
        ScriptJump(ctx, CeEventScript_CenterHealRefused);
}

void CeModes_ScriptRestHealGate(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (!CeModes_CanHeal(CE_HEAL_REST))
        ScriptJump(ctx, CeEventScript_RestHealRefused);
}

// Whether the wild battle that just ended was this area's encounter.
static bool32 WildBattleUsesArea(void)
{
    u32 i, count = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) ? 2 : 1;

    for (i = 0; i < count; i++)
    {
        struct Pokemon *foe = &gParties[B_TRAINER_OPPONENT_A][i];
        u16 species = GetMonData(foe, MON_DATA_SPECIES);

        if (species == SPECIES_NONE)
            continue;
        // A caught Pokémon is owned now, so the dupes clause can't describe it any more.
        if (gBattleOutcome == B_OUTCOME_CAUGHT && species == gBattleResults.caughtMonSpecies)
        {
            if (!(CeModes_Rules()->shinyClause && IsMonShiny(foe)))
                return TRUE;
        }
        else if (CeModes_CountsAsEncounter(species, IsMonShiny(foe)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

void CeModes_OnBattleEnd(void)
{
    enum CeBattleKind kind = CeModes_BattleKind();
    u32 i, sentOut = 0;

    if (CeModes_Rules()->nuzlocke && kind != CE_BATTLE_EXEMPT)
    {
        // Only Pokémon that fought can have fainted in this battle.
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (gBattleStruct->partyState[B_TRAINER_PLAYER][i].sentOut)
                sentOut |= 1u << i;
        }
        CeModes_MarkFaintedDead(sentOut);
        if (kind == CE_BATTLE_WILD && CeModes_CurrentArea() != CE_AREA_NONE && WildBattleUsesArea())
            CeModes_UseArea(CeModes_CurrentArea());
    }
    FlagClear(FLAG_CE_SCRIPTED_WILD_BATTLE);
}

void CeModes_AfterBattle(void)
{
    if (CeModes_BattleKind() != CE_BATTLE_EXEMPT)
        CeModes_ReleaseDead();
}
