#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "event_data.h"
#include "item.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script_pokemon_util.h"
#include "starter_choose.h"
#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/region_map_sections.h"

// Self-test for the game modes: starts a new game (copying the rules into the RAM save) and
// runs every rule on fixed scenarios. tools/canary/checks/modes.ts predicts each value from
// the settings and the tables the website wrote. Works on the RAM save only.

// The two Nuzlocke areas the scenario uses.
#define PROBE_MAPSEC_A MAPSEC_ROUTE_104
#define PROBE_MAPSEC_B MAPSEC_ROUTE_116

static void PutRules(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    CeSelfTest_LineStart("modes");
    CeSelfTest_PutField("hasRules", CeModes_SaveHasRules());
    CeSelfTest_PutField("hash", gSaveBlock2Ptr->pokedex.ceModes.configHash);
    CeSelfTest_PutField("nuzlocke", rules->nuzlocke);
    CeSelfTest_PutField("dupes", rules->dupesClause);
    CeSelfTest_PutField("shiny", rules->shinyClause);
    CeSelfTest_PutField("release", rules->releaseDead);
    CeSelfTest_PutField("nick", CeModes_ForcesNickname());
    CeSelfTest_PutField("noItems", rules->noBattleItems);
    CeSelfTest_PutField("setStyle", rules->forceSetStyle);
    CeSelfTest_PutField("mono", rules->monotype);
    CeSelfTest_PutField("only", rules->onlySpecies);
    CeSelfTest_PutField("limit", CeModes_PartyLimit());
    CeSelfTest_PutField("center", CeModes_CanHeal(CE_HEAL_CENTER));
    CeSelfTest_PutField("rest", CeModes_CanHeal(CE_HEAL_REST));
    CeSelfTest_LineEmit();
}

// Checksums over both website tables, as the game resolves them.
static void PutTables(void)
{
    u32 i, area, areaSum = 0, areaFree = 0, familySum = 0;

    for (i = 0; i < MAPSEC_COUNT; i++)
    {
        area = CeModes_AreaOfMapsec(i);
        if (area == CE_AREA_NONE)
            areaFree++;
        else
            areaSum += (area + 1) * (i + 1);
    }
    for (i = 0; i < NUM_SPECIES; i++)
        familySum += CeModes_FamilyRoot(i) * (i + 1);

    CeSelfTest_LineStart("modesTables");
    CeSelfTest_PutField("mapsecs", MAPSEC_COUNT);
    CeSelfTest_PutField("areaSum", areaSum);
    CeSelfTest_PutField("areaFree", areaFree);
    CeSelfTest_PutField("species", NUM_SPECIES);
    CeSelfTest_PutField("familySum", familySum);
    CeSelfTest_LineEmit();
}

// The species-level gates (monotype, single family) for one species, nothing owned yet.
static void PutSpecies(u16 species, u32 freshArea)
{
    CeSelfTest_LineStart("modesSpecies");
    CeSelfTest_PutField("species", species);
    CeSelfTest_PutField("root", CeModes_FamilyRoot(species));
    CeSelfTest_PutField("wild", CeModes_CatchRule(species, FALSE, freshArea, CE_BATTLE_WILD));
    CeSelfTest_PutField("scripted", CeModes_CatchRule(species, FALSE, freshArea, CE_BATTLE_SCRIPTED));
    CeSelfTest_PutField("trainer", CeModes_CatchRule(species, FALSE, freshArea, CE_BATTLE_TRAINER));
    CeSelfTest_PutField("counts", CeModes_CountsAsEncounter(species, FALSE));
    CeSelfTest_LineEmit();
}

// First encounter, a second one in the same area, then a dupe in a fresh area.
static void ProbeNuzlocke(u16 species)
{
    u32 areaA = CeModes_AreaOfMapsec(PROBE_MAPSEC_A);
    u32 areaB = CeModes_AreaOfMapsec(PROBE_MAPSEC_B);

    CeSelfTest_LineStart("modesNuzlocke");
    CeSelfTest_PutField("species", species);
    CeSelfTest_PutField("areaA", areaA);
    CeSelfTest_PutField("areaB", areaB);
    CeSelfTest_PutField("first", CeModes_CatchRule(species, FALSE, areaA, CE_BATTLE_WILD));
    CeModes_UseArea(areaA);
    CeSelfTest_PutField("usedA", CeModes_IsAreaUsed(areaA));
    CeSelfTest_PutField("second", CeModes_CatchRule(species, FALSE, areaA, CE_BATTLE_WILD));
    CeSelfTest_PutField("secondShiny", CeModes_CatchRule(species, TRUE, areaA, CE_BATTLE_WILD));
    CeSelfTest_PutField("secondScripted", CeModes_CatchRule(species, FALSE, areaA, CE_BATTLE_SCRIPTED));
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_SET_CAUGHT);
    CeSelfTest_PutField("dupe", CeModes_CatchRule(species, FALSE, areaB, CE_BATTLE_WILD));
    CeSelfTest_PutField("dupeCounts", CeModes_CountsAsEncounter(species, FALSE));
    CeSelfTest_PutField("dupeShiny", CeModes_CatchRule(species, TRUE, areaB, CE_BATTLE_WILD));
    CeSelfTest_LineEmit();
}

// A fainted party Pokémon: marked dead, not healed, not revived, then maybe released.
static void ProbeDeath(u16 species)
{
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u32 zero = 0, item = ITEM_ORAN_BERRY;

    ZeroPlayerPartyMons();
    CreateMon(&party[0], species, 5, 0, OTID_STRUCT_PRESET(0));
    CreateMon(&party[1], species, 5, 0, OTID_STRUCT_PRESET(0));
    CalculateMonStats(&party[0]);
    CalculateMonStats(&party[1]);
    SetMonData(&party[0], MON_DATA_HELD_ITEM, &item);
    SetMonData(&party[0], MON_DATA_HP, &zero);
    CalculatePlayerPartyCount();

    CeSelfTest_LineStart("modesDeath");
    CeSelfTest_PutField("marked", CeModes_MarkFaintedDead((1u << PARTY_SIZE) - 1));
    CeSelfTest_PutField("dead0", CeModes_IsDead(&party[0]));
    CeSelfTest_PutField("dead1", CeModes_IsDead(&party[1]));
    HealPlayerParty();
    CeSelfTest_PutField("healed0", GetMonData(&party[0], MON_DATA_HP) != 0);
    CeSelfTest_PutField("healed1", GetMonData(&party[1], MON_DATA_HP) != 0);
    SetMonData(&party[0], MON_DATA_HP, &zero);
    CeSelfTest_PutField("reviveFails", PokemonUseItemEffects(&party[0], ITEM_REVIVE, 0, 0, FALSE));
    CeSelfTest_PutField("revived0", GetMonData(&party[0], MON_DATA_HP) != 0);
    CeSelfTest_PutField("berries", CountTotalItemQuantityInBag(ITEM_ORAN_BERRY));
    CeSelfTest_PutField("released", CeModes_ReleaseDead());
    CeSelfTest_PutField("berriesAfter", CountTotalItemQuantityInBag(ITEM_ORAN_BERRY));
    CeSelfTest_PutField("party", CalculatePlayerPartyCount());
    CeSelfTest_PutField("deaths", gSaveBlock2Ptr->pokedex.ceModes.deaths);
    CeSelfTest_LineEmit();
}

// Three Pokémon given the way scripts and catches give them, into an empty party.
static void ProbePartyLimit(u16 species)
{
    struct Pokemon mon;

    ZeroPlayerPartyMons();
    CalculatePlayerPartyCount();
    CreateMon(&mon, species, 5, 0, OTID_STRUCT_PRESET(0));

    CeSelfTest_LineStart("modesParty");
    CeSelfTest_PutField("give0", GiveScriptedMonToPlayer(&mon, PARTY_SIZE));
    CeSelfTest_PutField("give1", GiveScriptedMonToPlayer(&mon, PARTY_SIZE));
    CeSelfTest_PutField("catch2", GiveCapturedMonToPlayer(&mon));
    CeSelfTest_PutField("party", CalculatePlayerPartyCount());
    CeSelfTest_LineEmit();
    ZeroPlayerPartyMons();
    CalculatePlayerPartyCount();
}

static void ProbeHardMode(void)
{
    CeSelfTest_LineStart("modesHard");
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    CeSelfTest_PutField("bagTrainer", IsAllowedToUseBag());
    CeSelfTest_PutField("potionTrainer", CeModes_BlocksBattleItem(ITEM_POTION));
    gBattleTypeFlags = 0;
    CeSelfTest_PutField("bagWild", IsAllowedToUseBag());
    CeSelfTest_PutField("potionWild", CeModes_BlocksBattleItem(ITEM_POTION));
    CeSelfTest_PutField("ballWild", CeModes_BlocksBattleItem(ITEM_POKE_BALL));
    gBattleTypeFlags = BATTLE_TYPE_FRONTIER;
    CeSelfTest_PutField("bagFrontier", IsAllowedToUseBag());
    gBattleTypeFlags = 0;
    CeSelfTest_PutField("styleLocked", CeOptions_IsLocked(CE_VOPT_BATTLE_STYLE));
    CeSelfTest_PutField("style", CeOptions_Effective(CE_VOPT_BATTLE_STYLE, OPTIONS_BATTLE_STYLE_SHIFT));
    CeSelfTest_LineEmit();
}

// Which battles the rules treat as exempt (rule-independent).
static void ProbeBattleKinds(void)
{
    static const u32 types[] = {
        0, BATTLE_TYPE_LEGENDARY, BATTLE_TYPE_ROAMER, BATTLE_TYPE_TRAINER, BATTLE_TYPE_SAFARI,
        BATTLE_TYPE_FIRST_BATTLE, BATTLE_TYPE_CATCH_TUTORIAL, BATTLE_TYPE_BATTLE_TOWER | BATTLE_TYPE_TRAINER,
        BATTLE_TYPE_PYRAMID, BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_TRAINER, BATTLE_TYPE_SECRET_BASE | BATTLE_TYPE_TRAINER,
        BATTLE_TYPE_LINK | BATTLE_TYPE_TRAINER,
    };
    u32 i, kinds = 0;

    for (i = 0; i < ARRAY_COUNT(types); i++)
    {
        gBattleTypeFlags = types[i];
        kinds |= CeModes_BattleKind() << (2 * i);
    }
    gBattleTypeFlags = 0;
    FlagSet(FLAG_CE_SCRIPTED_WILD_BATTLE);
    CeSelfTest_LineStart("modesKinds");
    CeSelfTest_PutField("kinds", kinds);
    CeSelfTest_PutField("scripted", CeModes_BattleKind());
    // An exempt battle's end touches nothing else (there is no gBattleStruct outside battle).
    gBattleTypeFlags = BATTLE_TYPE_SAFARI;
    CeModes_OnBattleEnd();
    gBattleTypeFlags = 0;
    CeSelfTest_PutField("afterEnd", CeModes_BattleKind());
    CeSelfTest_LineEmit();
}

void CeSelfTest_ProbeModes(void)
{
    const struct CeSelfTestConfig *config = &gCeRomConfig.body.selfTest;
    u32 i, freshArea;
    u16 starter = GetStarterPokemon(0);

    // A fresh run: nothing owned, no flags, rules copied from the ROM.
    memset(gSaveBlock1Ptr->dexCaught, 0, sizeof(gSaveBlock1Ptr->dexCaught));
    ZeroPlayerPartyMons();
    FlagSet(FLAG_SYS_POKEDEX_GET);
    gBattleTypeFlags = 0;
    CeModes_OnNewGame();

    PutRules();
    PutTables();
    // An area that is never used up in this probe.
    freshArea = CeModes_AreaOfMapsec(MAPSEC_ROUTE_101);
    for (i = 0; i < CE_SELF_TEST_SPECIES && config->species[i] != SPECIES_NONE; i++)
        PutSpecies(config->species[i], freshArea);
    for (i = 0; i < 3; i++)
        PutSpecies(GetStarterPokemon(i), freshArea);
    ProbeNuzlocke(starter);
    ProbeDeath(starter);
    ProbePartyLimit(starter);
    ProbeHardMode();
    ProbeBattleKinds();
}
