#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "roamer.h"
#include "script.h"
#include "starter_choose.h"
#include "constants/trade.h"
#include "ce/encounters.h"
#include "ce/selftest.h"

// Statics self-test: what the game reads for each encounter, through the code paths the
// map scripts and C sites use. Checked by tools/canary/checks/statics.ts.

extern const u8 CeSelfTest_EventScript_StaticsGift[];
extern const u8 CeSelfTest_EventScript_StaticsEgg[];
extern const u8 CeSelfTest_EventScript_StaticsWild[];
extern const u8 CeSelfTest_EventScript_StaticsEvent[];

// Not in any header: specials called from scripts.
u16 GetInGameTradeSpeciesInfo(void);
void LoadWallyZigzagoon(void);

static void PutMon(struct Pokemon *mon)
{
    CeSelfTest_PutField("species", GetMonData(mon, MON_DATA_SPECIES));
    CeSelfTest_PutField("level", GetMonData(mon, MON_DATA_LEVEL));
    CeSelfTest_PutField("item", GetMonData(mon, MON_DATA_HELD_ITEM));
    CeSelfTest_PutField("shiny", GetMonData(mon, MON_DATA_IS_SHINY));
    CeSelfTest_PutField("egg", GetMonData(mon, MON_DATA_IS_EGG));
}

// One line per mon a script or C site created. The probe name says which path made it.
static void ProbeMon(const char *probe, enum CeEncounterId id, struct Pokemon *mon)
{
    CeSelfTest_LineStart(probe);
    CeSelfTest_PutField("id", id);
    PutMon(mon);
    CeSelfTest_LineEmit();
}

static void ProbeScript(const char *probe, enum CeEncounterId id, const u8 *script, struct Pokemon *mon)
{
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    gSpecialVar_Result = 0xFFFF;
    RunScriptImmediately(script);
    CeSelfTest_LineStart(probe);
    CeSelfTest_PutField("id", id);
    CeSelfTest_PutField("result", gSpecialVar_Result);
    PutMon(mon);
    CeSelfTest_LineEmit();
}

void CeSelfTest_ProbeStatics(void)
{
    u32 id, slot;

    // Every encounter as ce_loadenc loads it.
    for (id = 0; id < CE_ENC_COUNT; id++)
    {
        CeEnc_LoadVars(id);
        CeSelfTest_LineStart("statics");
        CeSelfTest_PutField("id", id);
        CeSelfTest_PutField("species", VarGet(VAR_CE_ENC_SPECIES));
        CeSelfTest_PutField("level", VarGet(VAR_CE_ENC_LEVEL));
        CeSelfTest_PutField("item", VarGet(VAR_CE_ENC_ITEM));
        CeSelfTest_PutField("kind", gCeEncounterInfo[id].kind);
        CeSelfTest_PutField("map", gCeEncounterInfo[id].map);
        CeSelfTest_LineEmit();
    }

    // The script macros, run like a map script.
    ProbeScript("staticsGivemon", CE_ENC_BELDUM, CeSelfTest_EventScript_StaticsGift, &gParties[B_TRAINER_PLAYER][0]);
    ProbeScript("staticsGiveegg", CE_ENC_WYNAUT_EGG, CeSelfTest_EventScript_StaticsEgg, &gParties[B_TRAINER_PLAYER][0]);
    ProbeScript("staticsWild", CE_ENC_REGICE, CeSelfTest_EventScript_StaticsWild, &gParties[B_TRAINER_OPPONENT_A][0]);
    ProbeScript("staticsEvent", CE_ENC_LATIAS_SOUTHERN_ISLAND, CeSelfTest_EventScript_StaticsEvent, &gParties[B_TRAINER_OPPONENT_A][0]);

    // C sites.
    for (slot = 0; slot < 3; slot++)
    {
        ZeroPlayerPartyMons();
        CeEnc_GiveStarter(GetStarterPokemon(slot), slot);
        ProbeMon("staticsStarter", CE_ENC_STARTER_GRASS + slot, &gParties[B_TRAINER_PLAYER][0]);
    }

    ZeroPlayerPartyMons();
    LoadWallyZigzagoon();
    ProbeMon("staticsTutorial", CE_ENC_WALLY_ZIGZAGOON, &gParties[B_TRAINER_PLAYER][0]);
    ZeroPlayerPartyMons();

    for (slot = 0; slot < 2; slot++)
    {
        DeactivateAllRoamers();
        gSpecialVar_0x8004 = slot; // the TV multichoice: 0 = red (Latias), 1 = blue (Latios)
        InitRoamer();
        CeSelfTest_LineStart("staticsRoamer");
        CeSelfTest_PutField("id", slot == 0 ? CE_ENC_ROAMER_LATIAS : CE_ENC_ROAMER_LATIOS);
        CeSelfTest_PutField("species", gSaveBlock1Ptr->roamer[0].species);
        CeSelfTest_PutField("level", gSaveBlock1Ptr->roamer[0].level);
        CeSelfTest_LineEmit();
    }
    DeactivateAllRoamers();

    for (id = 0; id < CE_ENC_COUNT; id++)
    {
        if (gCeEncounterInfo[id].kind != CE_ENC_KIND_TRADE)
            continue;
        gSpecialVar_0x8005 = gCeEncounterInfo[id].trade;
        CeSelfTest_LineStart("staticsTrade");
        CeSelfTest_PutField("id", id);
        CeSelfTest_PutField("request", GetInGameTradeSpeciesInfo());
        CeSelfTest_PutField("species", CeEnc_TradeSpecies(gCeEncounterInfo[id].trade, SPECIES_NONE));
        CeSelfTest_LineEmit();
    }
    ZeroEnemyPartyMons();
}
