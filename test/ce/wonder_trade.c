#include "global.h"
#include "event_data.h"
#include "item.h"
#include "malloc.h"
#include "party_menu.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "string_util.h"
#include "trade.h"
#include "test/test.h"
#include "ce/option_menu_qol.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/team.h"
#include "ce/test.h"
#include "ce/third_type.h"
#include "ce/wonder_trade.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/party_menu.h"
#include "constants/region_map_sections.h"

// Wonder Trade (ce/wonder_trade.h): the pool, the gate, the received Pokémon, the checks on the
// sent one, and the helpers the trade scene's hooks call.

static void SetMoves(struct Pokemon *mon, u16 move)
{
    u16 none = MOVE_NONE;
    u32 i;

    SetMonData(mon, MON_DATA_MOVE1, &move);
    for (i = 1; i < MAX_MON_MOVES; i++)
        SetMonData(mon, MON_DATA_MOVE1 + i, &none);
}

// A party of `count` healthy Pokémon that know only Tackle, and empty PC boxes.
static void SetUpParty(u32 count)
{
    u32 i;

    ZeroPlayerPartyMons();
    ResetPokemonStorageSystem();
    for (i = 0; i < count; i++)
    {
        CreateMon(&gParties[B_TRAINER_PLAYER][i], SPECIES_ZIGZAGOON, 10 + i, 0, OTID_STRUCT_PLAYER_ID);
        SetMoves(&gParties[B_TRAINER_PLAYER][i], MOVE_TACKLE);
        CalculateMonStats(&gParties[B_TRAINER_PLAYER][i]);
    }
    CalculatePlayerPartyCount();
}

static void ChooseParty(u32 slot)
{
    gSpecialVar_0x8004 = slot;
}

static void ChoosePc(u32 box, u32 pos)
{
    gSpecialVar_0x8004 = PC_MON_CHOSEN;
    gSpecialVar_MonBoxId = box;
    gSpecialVar_MonBoxPos = pos;
}

static void PutInBox(u32 box, u32 pos, u16 species, u32 personality)
{
    struct Pokemon mon;

    CreateMon(&mon, species, 20, personality, OTID_STRUCT_PLAYER_ID);
    SetMoves(&mon, MOVE_TACKLE);
    CopyMon(GetBoxedMonPtr(box, pos), &mon.box, sizeof(mon.box));
}

static bool32 IsInBattleOnlyList(u16 species)
{
    u32 i;

    for (i = 0; i < CeWonderTrade_BattleOnlyCount(); i++)
    {
        if (CeWonderTrade_BattleOnlyForm(i) == species)
            return TRUE;
    }
    return FALSE;
}

TEST("CE: Wonder Trade's pool has only base and regional forms that can be traded, legendaries included")
{
    u32 s, count = 0;

    for (s = 1; s < NUM_SPECIES; s++)
    {
        const struct SpeciesInfo *info = &gSpeciesInfo[s];

        if (!CeWonderTrade_InPool(s, TYPE_NONE))
            continue;
        count++;
        EXPECT(IsSpeciesEnabled(s));
        EXPECT_NE((u32)info->natDexNum, 0);
        EXPECT(CeWonderTrade_GroupOf(s) == s || info->isAlolanForm || info->isGalarianForm || info->isHisuianForm || info->isPaldeanForm);
        EXPECT(!info->isMegaEvolution && !info->isPrimalReversion && !info->isUltraBurst && !info->isGigantamax && !info->isTeraForm && !info->isTotem);
        EXPECT(!info->cannotBeTraded);
        EXPECT(!IsInBattleOnlyList(s));
    }
    EXPECT_GT(count, 1000);

    // The engine's BATTLE_ONLY_FORMS has 33.
    EXPECT_EQ(CeWonderTrade_BattleOnlyCount(), 33);

    EXPECT(!CeWonderTrade_InPool(SPECIES_NONE, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_EGG, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_VENUSAUR_MEGA, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_KYOGRE_PRIMAL, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_CHARIZARD_GMAX, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_RATICATE_ALOLA_TOTEM, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_MIMIKYU_BUSTED, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_DARMANITAN_GALAR_ZEN, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_KYUREM_WHITE, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_ARCEUS_FIRE, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_ROTOM_WASH, TYPE_NONE));
    EXPECT(!CeWonderTrade_InPool(SPECIES_PIKACHU_STARTER, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_MEWTWO, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_RAYQUAZA, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_VULPIX_ALOLA, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_DARMANITAN_GALAR_STANDARD, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_ARCEUS_NORMAL, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_ROTOM, TYPE_NONE));
    EXPECT(CeWonderTrade_InPool(SPECIES_KYUREM, TYPE_NONE));
}

TEST("CE: Every species in Wonder Trade's pool belongs to exactly one Pokédex entry")
{
    u8 *seen = AllocZeroed(NUM_SPECIES);
    u8 *groups = AllocZeroed(NUM_SPECIES);
    u32 s, i, distinct = 0, inPool = 0;

    // Walk the heads as CountGroups does, counting where each species turns up.
    for (s = 1; s < NUM_SPECIES; s++)
    {
        const u16 *forms = gSpeciesInfo[s].formSpeciesIdTable;

        if (CeWonderTrade_GroupOf(s) != s)
            continue;
        if (forms == NULL)
            seen[s]++;
        else
            for (i = 0; forms[i] != FORM_SPECIES_END; i++)
                seen[forms[i]]++;
    }
    for (s = 1; s < NUM_SPECIES; s++)
    {
        if (!CeWonderTrade_InPool(s, TYPE_NONE))
            continue;
        EXPECT_EQ(seen[s], 1);
        inPool++;
        if (groups[CeWonderTrade_GroupOf(s)]++ == 0)
            distinct++;
    }
    EXPECT_EQ(CeWonderTrade_CountGroups(TYPE_NONE), distinct);
    // Fewer entries than species: the regional forms share theirs.
    EXPECT_LT(distinct, inPool);
    EXPECT_EQ(groups[SPECIES_VULPIX], 2);
    Free(seen);
    Free(groups);
}

TEST("CE: Wonder Trade picks a species from the pool, and only of the run's type in a monotype run")
{
    u32 i;
    u16 species;

    for (i = 0; i < 40; i++)
    {
        species = CeWonderTrade_PickSpecies(TYPE_NONE);
        EXPECT(CeWonderTrade_InPool(species, TYPE_NONE));
    }

    CeTest_StartRun(&(struct CeModesConfig) { .monotype = TYPE_GHOST });
    EXPECT_GT(CeWonderTrade_CountGroups(TYPE_GHOST), 10);
    EXPECT_LT(CeWonderTrade_CountGroups(TYPE_GHOST), CeWonderTrade_CountGroups(TYPE_NONE));
    for (i = 0; i < 100; i++)
    {
        species = CeWonderTrade_PickSpecies(CeModes_Rules()->monotype);
        EXPECT(CeWonderTrade_InPool(species, TYPE_GHOST));
        EXPECT(CeSpecies_IsOfType(species, TYPE_GHOST));
    }
}

TEST("CE: Wonder Trade refuses Nuzlocke, single-Pokémon and static team runs, and hides its attendant and option there")
{
    struct CeModesConfig rules = {0};
    bool32 staticTeam = FALSE;
    enum CeQolMode mode = CE_QOL_MODE_ON;
    u32 want = CE_WT_OPEN;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF;        want = CE_WT_REFUSED_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_OFF; want = CE_WT_REFUSED_OFF; }
    PARAMETRIZE { rules.nuzlocke = TRUE;         want = CE_WT_REFUSED_RULES; }
    PARAMETRIZE { rules.onlySpecies = SPECIES_RALTS; want = CE_WT_REFUSED_RULES; }
    PARAMETRIZE { rules.partyLimit = 1;          want = CE_WT_REFUSED_RULES; }
    PARAMETRIZE { staticTeam = TRUE;             want = CE_WT_REFUSED_STATIC_TEAM; }
    // The other modes may trade.
    PARAMETRIZE { }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_ON; }
    PARAMETRIZE { rules.noBattleItems = TRUE; rules.forceSetStyle = TRUE; }
    PARAMETRIZE { rules.noCenterHeal = TRUE; rules.noRestHeal = TRUE; }
    PARAMETRIZE { rules.noEvolution = TRUE; }
    PARAMETRIZE { rules.noExp = CE_NO_EXP_ALL; }
    PARAMETRIZE { rules.monotype = TYPE_DRAGON; }

    gSaveBlock2Ptr->pokedex.ceQolChosen[1] = gSaveBlock2Ptr->pokedex.ceQolValue[1] = 0;
    CeTest_SetQol(CE_QOL_WONDER_TRADE, mode);
    CeTest_StartRun(&rules);
    memset(&gSaveBlock2Ptr->pokedex.ceTeam, 0, sizeof(gSaveBlock2Ptr->pokedex.ceTeam));
    if (staticTeam)
    {
        gSaveBlock2Ptr->pokedex.ceTeam.magic = CE_TEAM_SAVE_MAGIC;
        gSaveBlock2Ptr->pokedex.ceTeam.version = CE_TEAM_SAVE_VERSION;
        gSaveBlock2Ptr->pokedex.ceTeam.staticTeam = TRUE;
    }
    EXPECT_EQ(CeWonderTrade_Block(), want);
    EXPECT_EQ(CeWonderTrade_IsAllowed(), (want == CE_WT_OPEN || want == CE_WT_REFUSED_OFF));

    // The attendant (FLAG_TEMP_C) stands upstairs exactly when the gate is open.
    FlagClear(FLAG_TEMP_C);
    CeProgression_UpdatePcSellers(TRUE);
    EXPECT_EQ(FlagGet(FLAG_TEMP_C), want != CE_WT_OPEN);
    FlagClear(FLAG_TEMP_C);
    CeProgression_UpdatePcSellers(FALSE);
    EXPECT(FlagGet(FLAG_TEMP_C));

    // Its MORE OPTIONS row is listed unless locked off, and never while the rules refuse it, even
    // when the ROM lists locked-off rows.
    CE_TEST_CONFIG->body.qol.showLockedOff = FALSE;
    EXPECT_EQ(CeOptions_IsQolRowShown(CE_QOL_WONDER_TRADE), mode != CE_QOL_MODE_OFF && CeWonderTrade_IsAllowed());
    CE_TEST_CONFIG->body.qol.showLockedOff = TRUE;
    EXPECT_EQ(CeOptions_IsQolRowShown(CE_QOL_WONDER_TRADE), CeWonderTrade_IsAllowed());
    // Other rows ignore the rules.
    CeTest_SetQol(CE_QOL_BATTLE_ITEMS, CE_QOL_MODE_PLAYER_ON);
    EXPECT(CeOptions_IsQolRowShown(CE_QOL_BATTLE_ITEMS));
}

TEST("CE: The Pokémon Wonder Trade sends comes from another trainer at the sent one's level")
{
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u32 i, perfect = 0;
    u16 species = SPECIES_NONE;

    PARAMETRIZE { species = SPECIES_ZIGZAGOON; }
    PARAMETRIZE { species = SPECIES_MEWTWO; }
    PARAMETRIZE { species = SPECIES_VULPIX_ALOLA; }

    CeWonderTrade_CreateMon(mon, species, 37);
    EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), species);
    EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), 37);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MET_LEVEL), 37);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MET_LOCATION), METLOC_IN_GAME_TRADE);
    EXPECT_NE(GetMonData(mon, MON_DATA_OT_ID), READ_OTID_FROM_SAVE);
    EXPECT(IsTradedMon(mon));
    GetMonData(mon, MON_DATA_OT_NAME, otName);
    EXPECT_NE(otName[0], EOS);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HELD_ITEM), ITEM_NONE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_MAIL), MAIL_NONE);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HP), GetMonData(mon, MON_DATA_MAX_HP));
    EXPECT_NE(GetMonData(mon, MON_DATA_MOVE1), MOVE_NONE);
    for (i = 0; i < NUM_STATS; i++)
    {
        u32 iv = GetMonData(mon, MON_DATA_HP_IV + i);
        EXPECT_LE(iv, MAX_PER_STAT_IVS);
        if (iv == MAX_PER_STAT_IVS)
            perfect++;
    }
    EXPECT_GE(perfect, gSpeciesInfo[species].perfectIVCount);
}

TEST("CE: Wonder Trade won't send an Egg, a Pokémon that can't be traded, Mail, an item with no room, the last Surf user or the last Pokémon")
{
    u16 item;

    // Two healthy party Pokémon: the first can go.
    SetUpParty(2);
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_OK);

    // An Egg.
    SetUpParty(2);
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_IS_EGG, &(bool8) { TRUE });
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_EGG);

    // A species that can't be traded.
    SetUpParty(2);
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_KYUREM_WHITE, 50, 0, OTID_STRUCT_PLAYER_ID);
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_CANT_TRADE);

    // Mail.
    SetUpParty(2);
    item = ITEM_ORANGE_MAIL;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM, &item);
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_MAIL);

    // A held item the bag has room for, then none.
    SetUpParty(2);
    item = ITEM_LEFTOVERS;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM, &item);
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_OK);
    while (AddBagItem(ITEM_LEFTOVERS, MAX_BAG_ITEM_CAPACITY))
        ;
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_BAG_FULL);

    // The party's last Pokémon that can battle, from the party; a PC Pokémon can always go.
    SetUpParty(1);
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_LAST_MON);
    SetUpParty(2);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HP, &(u16) { 0 });
    ChooseParty(0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_LAST_MON);
    ChooseParty(1);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_OK); // fainted, with a healthy one left
    SetUpParty(1);
    PutInBox(0, 0, SPECIES_ZIGZAGOON, 1);
    ChoosePc(0, 0);
    EXPECT_EQ(CeWonderTrade_CheckMon(), CE_WT_OK);
}

TEST("CE: Wonder Trade keeps the last Pokémon that knows Surf unless Surf works without it")
{
    u16 surf = MOVE_SURF;
    bool32 otherKnows = FALSE, canSurf = FALSE, fromPc = FALSE;

    PARAMETRIZE { }
    PARAMETRIZE { fromPc = TRUE; }
    PARAMETRIZE { otherKnows = TRUE; }
    PARAMETRIZE { fromPc = TRUE; otherKnows = TRUE; }
    PARAMETRIZE { canSurf = TRUE; }
    PARAMETRIZE { fromPc = TRUE; canSurf = TRUE; }

    SetUpParty(3);
    CeTest_SetQol(CE_QOL_FIELD_MOVES_WITHOUT_HMS, canSurf ? CE_QOL_MODE_ON : CE_QOL_MODE_OFF);
    if (canSurf)
        FlagSet(FLAG_BADGE05_GET);
    else
        FlagClear(FLAG_BADGE05_GET);
    if (fromPc)
    {
        PutInBox(3, 7, SPECIES_ZIGZAGOON, 1);
        SetBoxMonDataAt(3, 7, MON_DATA_MOVE1, &surf);
        ChoosePc(3, 7);
    }
    else
    {
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MOVE1, &surf);
        ChooseParty(0);
    }
    if (otherKnows)
    {
        // Another Pokémon in a box (for a party pick) or in the party (for a PC pick).
        if (fromPc)
            SetMonData(&gParties[B_TRAINER_PLAYER][2], MON_DATA_MOVE1, &surf);
        else
        {
            PutInBox(5, 0, SPECIES_ZIGZAGOON, 2);
            SetBoxMonDataAt(5, 0, MON_DATA_MOVE1, &surf);
        }
    }
    // An Egg that knows Surf doesn't count.
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_IS_EGG, &(bool8) { TRUE });
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_MOVE1, &surf);
    EXPECT_EQ(CeWonderTrade_CheckMon(), (otherKnows || canSurf) ? CE_WT_OK : CE_WT_LAST_SURFER);
}

TEST("CE: Wonder Trade puts the sent Pokémon's held item in the bag and readies the received one")
{
    struct ScriptContext ctx;
    u16 item = ITEM_LEFTOVERS;
    bool32 fromPc = FALSE;

    PARAMETRIZE { }
    PARAMETRIZE { fromPc = TRUE; }

    memset(&ctx, 0, sizeof(ctx));
    SetUpParty(2);
    FlagClear(FLAG_CE_WONDER_TRADE);
    if (fromPc)
    {
        PutInBox(2, 4, SPECIES_ZIGZAGOON, 1);
        SetBoxMonDataAt(2, 4, MON_DATA_HELD_ITEM, &item);
        ChoosePc(2, 4);
    }
    else
    {
        SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_HELD_ITEM, &item);
        ChooseParty(1);
    }
    CeTest_SetQol(CE_QOL_WONDER_TRADE, CE_QOL_MODE_ON);
    CeWonderTrade_ScriptPrepare(&ctx);
    EXPECT_EQ(gSpecialVar_Result, 2);
    EXPECT(CheckBagHasItem(ITEM_LEFTOVERS, 1));
    EXPECT_EQ(GetBoxMonData(GetSelectedBoxMonFromPcOrParty(), MON_DATA_HELD_ITEM), ITEM_NONE);
    EXPECT(FlagGet(FLAG_CE_WONDER_TRADE));
    EXPECT(CeWonderTrade_InPool(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), TYPE_NONE));
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), GetLevelFromBoxMonExp(GetSelectedBoxMonFromPcOrParty()));
    FlagClear(FLAG_CE_WONDER_TRADE);
}

TEST("CE: The trade scene's Wonder Trade hooks do nothing unless a Wonder Trade is playing")
{
    u8 otName[PLAYER_NAME_LENGTH + 1];
    u8 nickname[POKEMON_NAME_BUFFER_SIZE];
    bool32 fromPc = FALSE;

    PARAMETRIZE { }
    PARAMETRIZE { fromPc = TRUE; }

    SetUpParty(2);
    if (fromPc)
    {
        // The scene moved the PC pick here.
        CreateMon(&gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC], SPECIES_WINGULL, 20, 0, OTID_STRUCT_PLAYER_ID);
        ChoosePc(0, 0);
    }
    else
    {
        CreateMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_WINGULL, 20, 0, OTID_STRUCT_PLAYER_ID);
        ChooseParty(1);
    }
    CeWonderTrade_CreateMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MUDKIP, 20);

    FlagClear(FLAG_CE_WONDER_TRADE);
    StringCopy(gStringVar1, COMPOUND_STRING("UNTOUCHED"));
    EXPECT(!CeWonderTrade_BufferSceneStrings());
    EXPECT(!CeWonderTrade_BufferMonName());
    EXPECT(StringCompare(gStringVar1, COMPOUND_STRING("UNTOUCHED")) == 0);

    FlagSet(FLAG_CE_WONDER_TRADE);
    EXPECT(CeWonderTrade_BufferSceneStrings());
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_OT_NAME, otName);
    EXPECT(StringCompare(gStringVar1, otName) == 0);
    GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, nickname);
    EXPECT(StringCompare(gStringVar3, nickname) == 0);
    EXPECT(StringCompare(gStringVar2, GetSpeciesName(SPECIES_WINGULL)) == 0);
    // After the swap, the received Pokémon is where the sent one was.
    EXPECT(CeWonderTrade_BufferMonName());
    EXPECT(StringCompare(gStringVar1, GetSpeciesName(SPECIES_WINGULL)) == 0);
    EXPECT(StringCompare(gStringVar2, GetSpeciesName(SPECIES_WINGULL)) == 0);
    FlagClear(FLAG_CE_WONDER_TRADE);
}

TEST("CE: After a Wonder Trade from the PC, an evolved received Pokémon is written back to its box slot")
{
    struct Pokemon *received = &gParties[B_TRAINER_OPPONENT_A][TRADEMON_FROM_PC];
    bool32 sameMon = FALSE;

    PARAMETRIZE { sameMon = TRUE; }
    PARAMETRIZE { sameMon = FALSE; }

    SetUpParty(1);
    // CopyMonToPC put the received Kadabra in box 1, slot 3 before it evolved.
    PutInBox(1, 3, SPECIES_KADABRA, 0x1234);
    CreateMon(received, SPECIES_ALAKAZAM, 20, sameMon ? 0x1234 : 0x5678, OTID_STRUCT_PLAYER_ID);
    ChoosePc(1, 3);
    EXPECT_EQ(CeWonderTrade_FixPcEvolution(), sameMon);
    EXPECT_EQ(GetBoxMonDataAt(1, 3, MON_DATA_SPECIES), sameMon ? SPECIES_ALAKAZAM : SPECIES_KADABRA);

    // A party pick is never touched.
    ChooseParty(0);
    EXPECT(!CeWonderTrade_FixPcEvolution());
}
