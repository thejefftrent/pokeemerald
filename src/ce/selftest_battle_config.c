#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "config_changes.h"
#include "malloc.h"
#include "pokemon.h"
#include "pokerus.h"
#include "ce/selftest.h"
#include "constants/moves.h"

// Battle settings as the game reads them after the website patched sConfigChanges:
// a few values through the inline GetConfig() getters (the path battle code uses), two
// behaviours that depend on them, and a checksum over every tag via GetConfigInternal()
// so the canary covers the whole bitfield block. Then the type chart (ProbeTypeChart).

static u32 ChecksumAllConfigs(void)
{
    u32 hash = 0;
    enum ConfigTag tag;

    for (tag = 0; tag < CONFIG_COUNT; tag++)
        hash = hash * 31 + GetConfigInternal(tag);
    return hash;
}

// The type chart (gTypeEffectivenessTable, website-written): for every attacking type, which
// defending types GetTypeModifier() reads as x0, x0.5 and x2 (and any value that is none of
// x0, x0.5, x1 and x2), as bitmasks over the defending type ids. For the real attacking types
// also the multiplier the battle code works out (CalcTypeEffectivenessMultiplier, no abilities
// or items) against a battler of each single type. Then a checksum over every cell.
// tools/canary/checks/battle-config.ts predicts each line from the engine's type chart.
#define CHART_ATTACKER B_BATTLER_0
#define CHART_DEFENDER B_BATTLER_1

struct TypeChartMasks
{
    u32 zero, half, twice, other;
};

static void AddToMasks(struct TypeChartMasks *masks, u32 def, uq4_12_t mod)
{
    if (mod == UQ_4_12(0.0))
        masks->zero |= 1u << def;
    else if (mod == UQ_4_12(0.5))
        masks->half |= 1u << def;
    else if (mod == UQ_4_12(2.0))
        masks->twice |= 1u << def;
    else if (mod != UQ_4_12(1.0))
        masks->other |= 1u << def;
}

static uq4_12_t BattleMultiplier(enum Type atk, enum Type def)
{
    struct DamageContext ctx = {0};

    gBattleMons[CHART_DEFENDER].types[0] = def;
    gBattleMons[CHART_DEFENDER].types[1] = def;
    gBattleMons[CHART_DEFENDER].types[2] = TYPE_MYSTERY;
    ctx.battlerAtk = CHART_ATTACKER;
    ctx.battlerDef = CHART_DEFENDER;
    ctx.move = ctx.chosenMove = ctx.baseMove = MOVE_POUND;
    ctx.moveType = atk;
    return CalcTypeEffectivenessMultiplier(&ctx);
}

static void ProbeTypeChart(void)
{
    struct BattleStruct *saved = gBattleStruct;
    u32 hash = 0;
    u32 atk, def;

    // The battle code reads gBattleStruct (no gimmick active: all zero) and the field state.
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    if (gBattleStruct == NULL)
    {
        gBattleStruct = saved;
        return;
    }
    gFieldStatuses = 0;
    memset(&gBattleMons[CHART_ATTACKER], 0, sizeof(gBattleMons[CHART_ATTACKER]));
    memset(&gBattleMons[CHART_DEFENDER], 0, sizeof(gBattleMons[CHART_DEFENDER]));

    for (atk = 0; atk < NUMBER_OF_MON_TYPES; atk++)
    {
        struct TypeChartMasks chart = {0};
        struct TypeChartMasks battle = {0};
        // A Mystery move skips type calculation, and None and Stellar aren't move types here.
        bool32 real = atk != TYPE_NONE && atk != TYPE_MYSTERY && atk != TYPE_STELLAR;

        for (def = 0; def < NUMBER_OF_MON_TYPES; def++)
        {
            AddToMasks(&chart, def, GetTypeModifier(atk, def));
            if (real)
                AddToMasks(&battle, def, BattleMultiplier(atk, def));
        }
        CeSelfTest_LineStart("typeChart");
        CeSelfTest_PutField("atk", atk);
        CeSelfTest_PutField("t0", chart.zero);
        CeSelfTest_PutField("t5", chart.half);
        CeSelfTest_PutField("t2", chart.twice);
        CeSelfTest_PutField("tx", chart.other);
        if (real)
        {
            CeSelfTest_PutField("b0", battle.zero);
            CeSelfTest_PutField("b5", battle.half);
            CeSelfTest_PutField("b2", battle.twice);
            CeSelfTest_PutField("bx", battle.other);
        }
        CeSelfTest_LineEmit();
    }

    for (atk = 0; atk < NUMBER_OF_MON_TYPES; atk++)
    {
        for (def = 0; def < NUMBER_OF_MON_TYPES; def++)
            hash = hash * 31 + GetTypeModifier(atk, def);
    }
    CeSelfTest_LineStart("typeChartAll");
    CeSelfTest_PutField("count", NUMBER_OF_MON_TYPES);
    CeSelfTest_PutField("checksum", hash);
    CeSelfTest_LineEmit();

    memset(&gBattleMons[CHART_DEFENDER], 0, sizeof(gBattleMons[CHART_DEFENDER]));
    Free(gBattleStruct);
    gBattleStruct = saved;
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

    ProbeTypeChart();
}
