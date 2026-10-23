#include "global.h"
#include "battle_util.h"
#include "config_changes.h"
#include "pokemon.h"
#include "pokerus.h"
#include "test/test.h"

// Battle settings the website can change (the battle-config area writes sConfigChanges, which
// is CE_PATCHABLE). Test builds read a copy of that table that SetConfig/WITH_CONFIG edit, so
// these show that a changed value reaches the code at runtime, as a patched ROM's does.

extern const struct ConfigChanges sConfigChanges; // declared for release builds only

TEST("CE: Battle config: each test starts from the ROM's sConfigChanges")
{
    EXPECT_EQ(GetConfig(B_CRIT_CHANCE), sConfigChanges.critChance);
    EXPECT_EQ(GetConfig(POKERUS_ENABLED), sConfigChanges.pokerusEnabled);
    EXPECT_EQ(GetConfigInternal(CONFIG_B_BURN_DAMAGE), sConfigChanges.burnDamage);
}

TEST("CE: Battle config: B_CRIT_CHANCE picks the critical hit table")
{
    u32 gen = GEN_3, stage0 = 0, stage2 = 0;

    PARAMETRIZE { gen = GEN_3; stage0 = 16; stage2 = 4; }
    PARAMETRIZE { gen = GEN_6; stage0 = 16; stage2 = 2; }
    PARAMETRIZE { gen = GEN_7; stage0 = 24; stage2 = 2; }

    SetConfig(CONFIG_B_CRIT_CHANCE, gen);
    EXPECT_EQ(GetCriticalHitOdds(0), stage0);
    EXPECT_EQ(GetCriticalHitOdds(2), stage2);
}

TEST("CE: Battle config: POKERUS_ENABLED switches Pokérus")
{
    struct Pokemon mon;
    u32 strain = 1, daysLeft = 1;
    bool32 enabled = FALSE;

    PARAMETRIZE { enabled = FALSE; }
    PARAMETRIZE { enabled = TRUE; }

    SetConfig(CONFIG_POKERUS_ENABLED, enabled);
    CreateMon(&mon, SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PLAYER_ID);
    SetMonData(&mon, MON_DATA_POKERUS_STRAIN, &strain);
    SetMonData(&mon, MON_DATA_POKERUS_DAYS_LEFT, &daysLeft);
    EXPECT_EQ(CheckMonPokerus(&mon), enabled);
}
