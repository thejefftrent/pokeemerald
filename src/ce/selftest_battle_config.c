#include "global.h"
#include "battle_util.h"
#include "config_changes.h"
#include "pokemon.h"
#include "pokerus.h"
#include "ce/selftest.h"

// Battle settings as the game reads them after the website patched sConfigChanges:
// a few values through the inline GetConfig() getters (the path battle code uses), two
// behaviours that depend on them, and a checksum over every tag via GetConfigInternal()
// so the canary covers the whole bitfield block.

static u32 ChecksumAllConfigs(void)
{
    u32 hash = 0;
    enum ConfigTag tag;

    for (tag = 0; tag < CONFIG_COUNT; tag++)
        hash = hash * 31 + GetConfigInternal(tag);
    return hash;
}

static bool32 PokerusShows(void)
{
    struct Pokemon mon;
    u32 strain = 1;
    u32 daysLeft = 1;

    CreateMon(&mon, SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PRESET(0));
    SetMonData(&mon, MON_DATA_POKERUS_STRAIN, &strain);
    SetMonData(&mon, MON_DATA_POKERUS_DAYS_LEFT, &daysLeft);
    return CheckMonPokerus(&mon);
}

void CeSelfTest_ProbeBattleConfig(void)
{
    CeSelfTest_LineStart("battleConfig");
    CeSelfTest_PutField("B_CRIT_CHANCE", GetConfig(B_CRIT_CHANCE));
    CeSelfTest_PutField("B_CRIT_MULTIPLIER", GetConfig(B_CRIT_MULTIPLIER));
    CeSelfTest_PutField("B_BURN_DAMAGE", GetConfig(B_BURN_DAMAGE));
    CeSelfTest_PutField("B_SLEEP_TURNS", GetConfig(B_SLEEP_TURNS));
    CeSelfTest_PutField("B_EXP_CATCH", GetConfig(B_EXP_CATCH));
    CeSelfTest_PutField("POKERUS_ENABLED", GetConfig(POKERUS_ENABLED));
    CeSelfTest_PutField("AI_ROLL_ATTACKING", GetConfig(AI_ROLL_ATTACKING));
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("battleConfigUse");
    CeSelfTest_PutField("critOdds0", GetCriticalHitOdds(0));
    CeSelfTest_PutField("critOdds2", GetCriticalHitOdds(2));
    CeSelfTest_PutField("pokerus", PokerusShows());
    CeSelfTest_PutField("count", CONFIG_COUNT);
    CeSelfTest_PutField("checksum", ChecksumAllConfigs());
    CeSelfTest_LineEmit();
}
