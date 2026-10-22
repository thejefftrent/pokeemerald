#include "global.h"
#include "pokemon.h"
#include "starter_choose.h"
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

    LineStart();
    Put("CE:{\"probe\":\"done\"}");
    LineEmit();
    Exit(0);
}
