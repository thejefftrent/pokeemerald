#include "global.h"
#include "event_data.h"
#include "main.h"
#include "overworld.h"
#include "field_move.h"
#include "item.h"
#include "money.h"
#include "player_pc.h"
#include "pokemon.h"
#include "ce/field_moves.h"
#include "ce/key_items.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/selftest.h"
#include "ce/start.h"
#include "constants/field_move.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/vars.h"

// Self-test for the new-game inventory: puts the RAM save's bag, PC and money back to how
// NewGameInitData leaves them, runs CeStart_OnNewGame and prints every bag and PC slot.
// tools/canary/checks/start.ts compares them with settings.start. Also prints the gates of
// the field-move QoL features on a fixed scenario. Works on the RAM save only.

#define PROBE_VANILLA_MONEY 3000 // NewGameInitData
#define PROBE_WHERE_PC      POCKETS_COUNT

static void PutSlot(u32 where, u32 slot, u16 item, u16 quantity)
{
    CeSelfTest_LineStart("startSlot");
    CeSelfTest_PutField("where", where);
    CeSelfTest_PutField("slot", slot);
    CeSelfTest_PutField("item", item);
    CeSelfTest_PutField("qty", quantity);
    CeSelfTest_LineEmit();
}

static void ProbeInventory(void)
{
    u32 pocket, i, bagSlots = 0, pcSlots = 0;

    ClearBag();
    NewGameInitPCItems();
    SetMoney(&gSaveBlock1Ptr->money, PROBE_VANILLA_MONEY);
    CeStart_OnNewGame();

    for (pocket = 0; pocket < POCKETS_COUNT; pocket++)
    {
        for (i = 0; i < gBagPockets[pocket].capacity; i++)
        {
            struct ItemSlot slot = BagPocket_GetSlotData(&gBagPockets[pocket], i);
            if (slot.itemId == ITEM_NONE)
                continue;
            PutSlot(pocket, i, slot.itemId, slot.quantity);
            bagSlots++;
        }
    }
    for (i = 0; i < PC_ITEMS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->pcItems[i].itemId == ITEM_NONE)
            continue;
        PutSlot(PROBE_WHERE_PC, i, gSaveBlock1Ptr->pcItems[i].itemId, gSaveBlock1Ptr->pcItems[i].quantity);
        pcSlots++;
    }

    CeSelfTest_LineStart("startInventory");
    CeSelfTest_PutField("money", GetMoney(&gSaveBlock1Ptr->money));
    CeSelfTest_PutField("bagSlots", bagSlots);
    CeSelfTest_PutField("pcSlots", pcSlots);
    CeSelfTest_PutField("where", PROBE_WHERE_PC);
    CeSelfTest_LineEmit();
}

// The field-move features with every badge and without: whether a party of one Pokémon that
// knows no field move can Cut (checkfieldmove) and Surf, where its party menu would add Fly,
// how long an Egg cycle is and whether the Infinite Repel holds the repel counter.
static void ProbeFieldMoves(void)
{
    u32 i, withBadges, cut, surf, eggCycle;
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][0];
    u16 savedRepel = VarGet(VAR_REPEL_STEP_COUNT);

    ZeroPlayerPartyMons();
    CreateMon(mon, SPECIES_ZIGZAGOON, 5, 0, OTID_STRUCT_PRESET(0));
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 none = MOVE_NONE;
        SetMonData(mon, MON_DATA_MOVE1 + i, &none);
    }
    gPartiesCount[B_TRAINER_PLAYER] = 1;

    for (withBadges = 0; withBadges <= 1; withBadges++)
    {
        for (i = 0; i < NUM_BADGES; i++)
        {
            if (withBadges)
                FlagSet(FLAG_BADGE01_GET + i);
            else
                FlagClear(FLAG_BADGE01_GET + i);
        }
        gSpecialVar_Result = PARTY_SIZE;
        CeFieldMoves_CheckFieldMove(FIELD_MOVE_CUT);
        cut = gSpecialVar_Result != PARTY_SIZE;
        surf = CeFieldMoves_CanSurfWithoutMove();

        CeSelfTest_LineStart("fieldMoves");
        CeSelfTest_PutField("on", CeQol_IsOn(CE_QOL_FIELD_MOVES_WITHOUT_HMS));
        CeSelfTest_PutField("badges", withBadges);
        CeSelfTest_PutField("cut", cut);
        CeSelfTest_PutField("surf", surf);
        CeSelfTest_LineEmit();
    }
    for (i = 0; i < NUM_BADGES; i++)
        FlagClear(FLAG_BADGE01_GET + i);
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;

    eggCycle = CeProgression_EggCycleLength(256);
    VarSet(VAR_REPEL_STEP_COUNT, CE_INFINITE_REPEL_STEPS);
    CeSelfTest_LineStart("qolExtras");
    CeSelfTest_PutField("eggCycle256", eggCycle);
    CeSelfTest_PutField("infiniteRepel", CeKeyItems_IsInfiniteRepelOn());
    CeSelfTest_PutField("repelItem", ITEM_CE_INFINITE_REPEL);
    CeSelfTest_LineEmit();
    VarSet(VAR_REPEL_STEP_COUNT, savedRepel);
}

// For tools/canary/field-moves.ts, which plays a generated ROM in headless mGBA: where the Lua
// script finds the main callback, the save and the party, the code it jumps to for a new game
// and a map load, the maps it uses (Route 103, Granite Cave), and a Zigzagoon that knows only
// Tackle (its bytes:
// personality and OT id 0, so the Pokémon isn't encrypted and is valid in any run).
#define SCENARIO_MON_WORDS_PER_LINE 8

static void ProbeScenario(void)
{
    struct Pokemon mon;
    const u32 *words = (const u32 *)&mon;
    u32 i, j;
    u16 move = MOVE_TACKLE, none = MOVE_NONE;
    u8 pp = 35;

    CeSelfTest_LineStart("scenarioCode");
    CeSelfTest_PutField("cb2", (u32)&gMain.callback2);
    CeSelfTest_PutField("state", (u32)&gMain.state);
    CeSelfTest_PutField("newGame", (u32)CB2_NewGame);
    CeSelfTest_PutField("loadMap", (u32)CB2_LoadMap);
    CeSelfTest_PutField("overworld", (u32)CB2_Overworld);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioSave");
    CeSelfTest_PutField("sb1Ptr", (u32)&gSaveBlock1Ptr);
    CeSelfTest_PutField("flags", offsetof(struct SaveBlock1, flags));
    CeSelfTest_PutField("location", offsetof(struct SaveBlock1, location));
    CeSelfTest_PutField("pos", offsetof(struct SaveBlock1, pos));
    CeSelfTest_PutField("party", (u32)&gParties[B_TRAINER_PLAYER][0]);
    CeSelfTest_PutField("partyCount", (u32)&gPartiesCount[B_TRAINER_PLAYER]);
    CeSelfTest_PutField("monSize", sizeof(struct Pokemon));
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioMap");
    CeSelfTest_PutField("group", MAP_GROUP(MAP_ROUTE103));
    CeSelfTest_PutField("num", MAP_NUM(MAP_ROUTE103));
    CeSelfTest_PutField("badge1", FLAG_BADGE01_GET);
    CeSelfTest_PutField("pokemonGet", FLAG_SYS_POKEMON_GET);
    CeSelfTest_PutField("sb2Ptr", (u32)&gSaveBlock2Ptr);
    CeSelfTest_PutField("playerName", offsetof(struct SaveBlock2, playerName));
    CeSelfTest_PutField("caveGroup", MAP_GROUP(MAP_GRANITE_CAVE_1F));
    CeSelfTest_PutField("caveNum", MAP_NUM(MAP_GRANITE_CAVE_1F));
    CeSelfTest_LineEmit();

    CreateMon(&mon, SPECIES_ZIGZAGOON, 20, 0, OTID_STRUCT_PRESET(0));
    SetMonData(&mon, MON_DATA_MOVE1, &move);
    SetMonData(&mon, MON_DATA_PP1, &pp);
    for (i = 1; i < MAX_MON_MOVES; i++)
        SetMonData(&mon, MON_DATA_MOVE1 + i, &none);
    CalculateMonStats(&mon);
    for (i = 0; i < sizeof(mon) / 4; i += SCENARIO_MON_WORDS_PER_LINE)
    {
        static const char *const names[SCENARIO_MON_WORDS_PER_LINE] = { "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7" };
        CeSelfTest_LineStart("scenarioMon");
        CeSelfTest_PutField("i", i);
        for (j = 0; j < SCENARIO_MON_WORDS_PER_LINE && i + j < sizeof(mon) / 4; j++)
            CeSelfTest_PutField(names[j], words[i + j]);
        CeSelfTest_LineEmit();
    }
}

void CeSelfTest_ProbeStart(void)
{
    ProbeInventory();
    ProbeFieldMoves();
    ProbeScenario();
}
