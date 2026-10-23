#include "global.h"
#include "event_data.h"
#include "script.h"
#include "ce/logic.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "constants/flags.h"
#include "constants/map_groups.h"

// Self-test for the key-item logic: prints gCeRomConfig.body.logic and runs the three Mr. Briney
// hooks on the RAM save, as Norman's win and a Pokémon Center would. tools/canary/checks/logic.ts
// compares them with settings.logic. Works on the RAM save only, and puts back what it changed.

extern const u8 EventScript_HideMrBriney[];
extern const u8 Common_EventScript_NopReturn[];

static const u8 sProbeScript[] = { 0x02 }; // where the context starts: 'end'

static void ResetContext(struct ScriptContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->scriptPtr = sProbeScript;
}

static bool32 HealsInDewford(void)
{
    return gSaveBlock1Ptr->lastHealLocation.mapGroup == MAP_GROUP(MAP_DEWFORD_TOWN)
        && gSaveBlock1Ptr->lastHealLocation.mapNum == MAP_NUM(MAP_DEWFORD_TOWN);
}

void CeSelfTest_ProbeLogic(void)
{
    struct ScriptContext ctx;
    struct WarpData savedHeal = gSaveBlock1Ptr->lastHealLocation;
    bool32 normanWasDefeated = FlagGet(FLAG_DEFEATED_PETALBURG_GYM);
    u32 hides, healReset, centersStop;

    // Norman's win: does it hide Mr. Briney?
    ResetContext(&ctx);
    CeLogic_ScriptNormanHideMrBriney(&ctx);
    hides = ctx.scriptPtr == EventScript_HideMrBriney;

    // Norman's win: does a Dewford heal location move to Petalburg?
    gSaveBlock1Ptr->lastHealLocation.mapGroup = MAP_GROUP(MAP_DEWFORD_TOWN);
    gSaveBlock1Ptr->lastHealLocation.mapNum = MAP_NUM(MAP_DEWFORD_TOWN);
    ResetContext(&ctx);
    CeLogic_ScriptNormanResetHealLocation(&ctx);
    healReset = !HealsInDewford();
    gSaveBlock1Ptr->lastHealLocation = savedHeal;

    // A Pokémon Center after Norman: does it stop keeping track of where he is?
    FlagSet(FLAG_DEFEATED_PETALBURG_GYM);
    ResetContext(&ctx);
    CeLogic_ScriptBrineyGoneAfterNorman(&ctx);
    centersStop = ctx.scriptPtr == Common_EventScript_NopReturn;
    if (!normanWasDefeated)
        FlagClear(FLAG_DEFEATED_PETALBURG_GYM);

    CeSelfTest_LineStart("logic");
    CeSelfTest_PutField("keepBriney", gCeRomConfig.body.logic.keepBriney);
    CeSelfTest_PutField("hidesBriney", hides);
    CeSelfTest_PutField("resetsDewfordHeal", healReset);
    CeSelfTest_PutField("centersStop", centersStop);
    CeSelfTest_LineEmit();
}
