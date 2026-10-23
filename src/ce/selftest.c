#include "global.h"
#include "pokemon.h"
#include "caps.h"
#include "event_data.h"
#include "starter_choose.h"
#include "ce/level_caps.h"
#include "ce/qol.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"

// Talks to mGBA's debug output directly so it also works in NDEBUG (release) builds.
#define REG_CE_DEBUG_ENABLE ((vu16 *)0x4FFF780)
#define REG_CE_DEBUG_FLAGS  ((vu16 *)0x4FFF700)
#define REG_CE_DEBUG_STRING ((char *)0x4FFF600)
#define CE_DEBUG_LEVEL_INFO 3
#define CE_LINE_MAX 255
#define CE_STARTER_COUNT 3 // STARTER_MON_COUNT is private to starter_choose.c

// Lines are built directly in mGBA's debug string buffer, so this costs no RAM.
static EWRAM_DATA u32 sLineLength = 0;

static void LineStart(void)
{
    sLineLength = 0;
}

static void Put(const char *s)
{
    while (*s != '\0' && sLineLength < CE_LINE_MAX)
        REG_CE_DEBUG_STRING[sLineLength++] = *s++;
}

static void PutU32(u32 value)
{
    char digits[11];
    u32 n = 0;
    do
    {
        digits[n++] = '0' + value % 10;
        value /= 10;
    } while (value != 0);
    while (n > 0 && sLineLength < CE_LINE_MAX)
        REG_CE_DEBUG_STRING[sLineLength++] = digits[--n];
}

static void PutField(const char *name, u32 value)
{
    Put(",\"");
    Put(name);
    Put("\":");
    PutU32(value);
}

static void LineEmit(void)
{
    REG_CE_DEBUG_STRING[sLineLength] = '\0';
    *REG_CE_DEBUG_FLAGS = CE_DEBUG_LEVEL_INFO | 0x100;
}

static void ProbeSpecies(enum Species species)
{
    struct Pokemon mon;
    u32 iv = MAX_PER_STAT_IVS;
    u32 ev = 0;
    u32 stat;

    // Personality 0 gives a neutral nature (Hardy), so stats depend only on base stats.
    CreateMon(&mon, species, MAX_LEVEL, 0, OTID_STRUCT_PRESET(0));
    for (stat = 0; stat < NUM_STATS; stat++)
    {
        SetMonData(&mon, MON_DATA_HP_IV + stat, &iv);
        SetMonData(&mon, MON_DATA_HP_EV + stat, &ev);
    }
    CalculateMonStats(&mon);

    LineStart();
    Put("CE:{\"probe\":\"species\"");
    PutField("species", species);
    PutField("baseHP", GetSpeciesBaseHP(species));
    PutField("type1", GetSpeciesType(species, 0));
    PutField("type2", GetSpeciesType(species, 1));
    PutField("ability0", GetSpeciesAbility(species, 0));
    PutField("ability1", GetSpeciesAbility(species, 1));
    PutField("ability2", GetSpeciesAbility(species, 2));
    PutField("catchRate", gSpeciesInfo[species].catchRate);
    PutField("maxHP", GetMonData(&mon, MON_DATA_MAX_HP));
    PutField("atk", GetMonData(&mon, MON_DATA_ATK));
    PutField("def", GetMonData(&mon, MON_DATA_DEF));
    PutField("spe", GetMonData(&mon, MON_DATA_SPEED));
    PutField("spa", GetMonData(&mon, MON_DATA_SPATK));
    PutField("spd", GetMonData(&mon, MON_DATA_SPDEF));
    Put("}");
    LineEmit();
}

static void ProbeStarters(void)
{
    u32 i;
    LineStart();
    Put("CE:{\"probe\":\"starters\"");
    for (i = 0; i < CE_STARTER_COUNT; i++)
    {
        static const char *const names[] = { "s0", "s1", "s2" };
        PutField(names[i], GetStarterPokemon(i));
    }
    Put("}");
    LineEmit();
}

static void PutQolMask(void)
{
    u32 word, i;
    for (word = 0; word < 2; word++)
    {
        u32 mask = 0;
        for (i = 0; i < 32 && word * 32 + i < CE_QOL_COUNT; i++)
        {
            if (CeQol_IsOn(word * 32 + i))
                mask |= 1u << i;
        }
        PutField(word == 0 ? "lo" : "hi", mask);
    }
}

// Which features the game sees as on, then again after the player flips every
// player's-choice feature (locked ones must not move).
static void ProbeQol(void)
{
    u32 i;

    LineStart();
    Put("CE:{\"probe\":\"qol\"");
    PutField("count", CE_QOL_COUNT);
    PutQolMask();
    Put("}");
    LineEmit();

    for (i = 0; i < CE_QOL_COUNT; i++)
    {
        if (CeQol_IsPlayerChoice(i))
            CeQol_SetPlayerValue(i, !CeQol_IsOn(i));
    }
    LineStart();
    Put("CE:{\"probe\":\"qolFlipped\"");
    PutQolMask();
    Put("}");
    LineEmit();

    LineStart();
    Put("CE:{\"probe\":\"options\"");
    PutField("textSpeed", CeOptions_Effective(CE_VOPT_TEXT_SPEED, 0));
    PutField("battleScene", CeOptions_Effective(CE_VOPT_BATTLE_SCENE, 0));
    PutField("battleStyle", CeOptions_Effective(CE_VOPT_BATTLE_STYLE, 0));
    Put("}");
    LineEmit();
}

// Level caps at three points of progress: nothing beaten, the first boss beaten,
// everything beaten. Works on the in-RAM save only.
static void ProbeLevelCaps(void)
{
    const u16 *bosses;
    u32 count = CeLevelCap_GetBosses(&bosses);
    u32 i;
    u16 boss;

    for (i = 0; i < count; i++)
        FlagClear(TRAINER_FLAGS_START + bosses[i]);

    LineStart();
    Put("CE:{\"probe\":\"caps\"");
    PutField("bossCount", count);
    PutField("cap0", CeLevelCap_Compute(&boss));
    PutField("boss0", boss);
    PutField("effective0", GetCurrentLevelCap());
    FlagSet(TRAINER_FLAGS_START + bosses[0]);
    PutField("cap1", CeLevelCap_Compute(&boss));
    PutField("boss1", boss);
    for (i = 0; i < count; i++)
        FlagSet(TRAINER_FLAGS_START + bosses[i]);
    PutField("capAll", CeLevelCap_Compute(&boss));
    PutField("bossAll", boss);
    Put("}");
    LineEmit();

    for (i = 0; i < count; i++)
        FlagClear(TRAINER_FLAGS_START + bosses[i]);
}

void CeSelfTest_LineStart(const char *probe)
{
    LineStart();
    Put("CE:{\"probe\":\"");
    Put(probe);
    Put("\"");
}

void CeSelfTest_PutField(const char *name, u32 value)
{
    PutField(name, value);
}

void CeSelfTest_LineEmit(void)
{
    Put("}");
    LineEmit();
}

static void __attribute__((noreturn)) Exit(u32 code)
{
    register u32 exitCode asm("r0") = code;
    asm volatile("swi 0x3" :: "r" (exitCode));
    while (1)
        ;
}

void CeSelfTest_RunIfEnabled(void)
{
    u32 i;
    const struct CeSelfTestConfig *config = &gCeRomConfig.body.selfTest;

    if (gCeRomConfig.hdr.selfTestMode == 0)
        return;

    *REG_CE_DEBUG_ENABLE = 0xC0DE;
    LineStart();
    Put("CE:{\"probe\":\"start\"");
    PutField("layout", gCeRomConfig.hdr.layoutVersion);
    PutField("configHash", gCeRomConfig.hdr.configHash);
    Put("}");
    LineEmit();

    for (i = 0; i < CE_SELF_TEST_SPECIES && config->species[i] != SPECIES_NONE; i++)
        ProbeSpecies(config->species[i]);
    ProbeStarters();
    ProbeLevelCaps();
    ProbeQol();
    if (CeSelfTest_ProbeLearnsets != NULL)
        CeSelfTest_ProbeLearnsets();
    if (CeSelfTest_ProbeTrainers != NULL)
        CeSelfTest_ProbeTrainers();
    if (CeSelfTest_ProbeWild != NULL)
        CeSelfTest_ProbeWild();
    if (CeSelfTest_ProbeStatics != NULL)
        CeSelfTest_ProbeStatics();
    if (CeSelfTest_ProbeModes != NULL)
        CeSelfTest_ProbeModes();
    if (CeSelfTest_ProbeBattleConfig != NULL)
        CeSelfTest_ProbeBattleConfig();
    if (CeSelfTest_ProbeItems != NULL)
        CeSelfTest_ProbeItems();
    if (CeSelfTest_ProbeStart != NULL)
        CeSelfTest_ProbeStart();
    if (CeSelfTest_ProbeSeedHash != NULL)
        CeSelfTest_ProbeSeedHash();
    if (CeSelfTest_ProbeTeam != NULL)
        CeSelfTest_ProbeTeam();
    if (CeSelfTest_ProbeLogic != NULL)
        CeSelfTest_ProbeLogic();

    LineStart();
    Put("CE:{\"probe\":\"done\"}");
    LineEmit();
    Exit(0);
}
