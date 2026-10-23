#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "malloc.h"
#include "pokemon.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "ce/third_type.h"
#include "constants/battle.h"
#include "constants/moves.h"

// Self-test for innate third types. For every probed species it prints the third type the
// game reads, the three types a battler gets when it is sent out (PokemonToBattleMon) and the
// Stealth Rock damage it takes. For species with a third type it then prints the battle
// type-effectiveness multiplier of every attacking type, from the real battle code
// (CalcTypeEffectivenessMultiplier on gBattleMons). tools/canary/checks/third-type.ts predicts
// each value from the ROM's own type chart.

#define PROBE_ATTACKER B_BATTLER_0
#define PROBE_DEFENDER B_BATTLER_1
#define PROBE_LEVEL    50

static void ProbeEffectiveness(enum Species species)
{
    u32 type;

    for (type = TYPE_NONE + 1; type < NUMBER_OF_MON_TYPES; type++)
    {
        struct DamageContext ctx = {0};

        if (type == TYPE_MYSTERY || type == TYPE_STELLAR)
            continue;
        // No abilities or held items, so the multiplier depends on the types alone.
        ctx.battlerAtk = PROBE_ATTACKER;
        ctx.battlerDef = PROBE_DEFENDER;
        ctx.move = ctx.chosenMove = ctx.baseMove = MOVE_POUND;
        ctx.moveType = type;
        CeSelfTest_LineStart("thirdTypeEff");
        CeSelfTest_PutField("species", species);
        CeSelfTest_PutField("atk", type);
        CeSelfTest_PutField("mod", CalcTypeEffectivenessMultiplier(&ctx));
        CeSelfTest_LineEmit();
    }
}

static void ProbeSpecies(enum Species species)
{
    struct Pokemon mon;

    CreateMon(&mon, species, PROBE_LEVEL, 0, OTID_STRUCT_PRESET(0));
    CalculateMonStats(&mon);
    PokemonToBattleMon(&mon, &gBattleMons[PROBE_DEFENDER]);

    CeSelfTest_LineStart("thirdType");
    CeSelfTest_PutField("species", species);
    CeSelfTest_PutField("stored", gCeSpeciesThirdType[species]);
    CeSelfTest_PutField("type3", CeSpecies_GetThirdType(species));
    CeSelfTest_PutField("battle0", gBattleMons[PROBE_DEFENDER].types[0]);
    CeSelfTest_PutField("battle1", gBattleMons[PROBE_DEFENDER].types[1]);
    CeSelfTest_PutField("battle2", gBattleMons[PROBE_DEFENDER].types[2]);
    CeSelfTest_PutField("maxHP", gBattleMons[PROBE_DEFENDER].maxHP);
    CeSelfTest_PutField("rocks", GetStealthHazardDamage(TYPE_SIDE_HAZARD_POINTED_STONES, PROBE_DEFENDER));
    CeSelfTest_LineEmit();

    if (CeSpecies_GetThirdType(species) != TYPE_MYSTERY)
        ProbeEffectiveness(species);
}

void CeSelfTest_ProbeThirdType(void)
{
    const struct CeSelfTestConfig *config = &gCeRomConfig.body.selfTest;
    struct BattleStruct *saved = gBattleStruct;
    u32 i;

    // The battle code reads gBattleStruct (no gimmick active: all zero) and the field state.
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    if (gBattleStruct == NULL)
    {
        gBattleStruct = saved;
        return;
    }
    gFieldStatuses = 0;
    memset(&gBattleMons[PROBE_ATTACKER], 0, sizeof(gBattleMons[PROBE_ATTACKER]));

    for (i = 0; i < CE_SELF_TEST_SPECIES && config->species[i] != SPECIES_NONE; i++)
        ProbeSpecies(config->species[i]);

    Free(gBattleStruct);
    gBattleStruct = saved;
}
