#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "daycare.h"
#include "event_data.h"
#include "item.h"
#include "player_pc.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script_pokemon_util.h"
#include "starter_choose.h"
#include "string_util.h"
#include "ce/modes.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "ce/team.h"
#include "ce/team_table.h"
#include "constants/battle.h"
#include "constants/flags.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/species.h"

// Self-test for the static team: starts a new game in the RAM save (fixed player name and
// trainer id), prints the party, the PC and the item PC, then earns the badges one by one and
// prints every delivery the PC makes. When two or more PC Pokémon wait for badges, the last
// one's slot is taken first, so it must land in the first free slot instead. Then the gates:
// catching, gifts, eggs, the starter and evolutions. tools/canary/checks/team.ts predicts
// every value from the settings. Runs last, on the RAM save only.

#define PROBE_OT_ID       0x2468ACE1
#define PROBE_PLACEHOLDER SPECIES_ZIGZAGOON
#define PROBE_EVO_SPECIES SPECIES_CATERPIE
#define PROBE_EVO_LEVEL   50

static const u8 sProbePlayerName[] = _("CE");

static u32 NameHash(const u8 *name, u32 length)
{
    u32 i, hash = 0;

    for (i = 0; i < length && name[i] != EOS; i++)
        hash += name[i] * (i + 1);
    return hash;
}

static bool32 IsPlayersOwn(struct Pokemon *mon)
{
    u8 name[PLAYER_NAME_LENGTH + 1];

    GetMonData(mon, MON_DATA_OT_NAME, name);
    return GetMonData(mon, MON_DATA_OT_ID) == READ_OTID_FROM_SAVE
        && StringCompareN(name, gSaveBlock2Ptr->playerName, PLAYER_NAME_LENGTH) == 0
        && GetMonData(mon, MON_DATA_OT_GENDER) == gSaveBlock2Ptr->playerGender;
}

// Two lines per Pokémon: who it is, then moves and stats. `where` is the party slot, or
// 100 + box * IN_BOX_COUNT + slot for the PC.
static void PutMon(u32 where, struct Pokemon *mon)
{
    u8 nickname[POKEMON_NAME_LENGTH + 1];
    u32 i, ivs = 0;

    GetMonData(mon, MON_DATA_NICKNAME, nickname);
    CeSelfTest_LineStart("teamMon");
    CeSelfTest_PutField("where", where);
    CeSelfTest_PutField("species", GetMonData(mon, MON_DATA_SPECIES));
    CeSelfTest_PutField("level", GetMonData(mon, MON_DATA_LEVEL));
    CeSelfTest_PutField("item", GetMonData(mon, MON_DATA_HELD_ITEM));
    CeSelfTest_PutField("ability", GetMonAbility(mon));
    CeSelfTest_PutField("nature", GetNature(mon));
    CeSelfTest_PutField("ball", GetMonData(mon, MON_DATA_POKEBALL));
    CeSelfTest_PutField("friendship", GetMonData(mon, MON_DATA_FRIENDSHIP));
    CeSelfTest_PutField("shiny", IsMonShiny(mon));
    CeSelfTest_PutField("gender", GetMonGender(mon));
    CeSelfTest_PutField("tera", GetMonData(mon, MON_DATA_TERA_TYPE));
    CeSelfTest_PutField("nick", NameHash(nickname, POKEMON_NAME_LENGTH));
    CeSelfTest_PutField("own", IsPlayersOwn(mon));
    CeSelfTest_PutField("fateful", GetMonData(mon, MON_DATA_MODERN_FATEFUL_ENCOUNTER));
    CeSelfTest_LineEmit();

    for (i = 0; i < NUM_STATS; i++)
        ivs |= GetMonData(mon, MON_DATA_HP_IV + i) << (5 * i);
    CeSelfTest_LineStart("teamMonStats");
    CeSelfTest_PutField("where", where);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        static const char *const names[MAX_MON_MOVES] = { "m0", "m1", "m2", "m3" };
        CeSelfTest_PutField(names[i], GetMonData(mon, MON_DATA_MOVE1 + i));
    }
    CeSelfTest_PutField("ivs", ivs);
    for (i = 0; i < NUM_STATS; i++)
    {
        static const char *const names[NUM_STATS] = { "evHp", "evAtk", "evDef", "evSpe", "evSpa", "evSpd" };
        CeSelfTest_PutField(names[i], GetMonData(mon, MON_DATA_HP_EV + i));
    }
    CeSelfTest_PutField("fullHp", GetMonData(mon, MON_DATA_HP) == GetMonData(mon, MON_DATA_MAX_HP));
    CeSelfTest_LineEmit();
}

static void PutBoxMon(u32 box, u32 slot)
{
    struct Pokemon mon;

    BoxMonAtToMon(box, slot, &mon);
    PutMon(100 + box * IN_BOX_COUNT + slot, &mon);
}

static void PutSave(const char *probe)
{
    struct CeTeamSave *save = &gSaveBlock2Ptr->pokedex.ceTeam;

    CeSelfTest_LineStart(probe);
    CeSelfTest_PutField("setUp", CeTeam_SaveIsSetUp());
    CeSelfTest_PutField("static", CeTeam_IsStaticTeam());
    CeSelfTest_PutField("badges", CeTeam_BadgeCount());
    CeSelfTest_PutField("mons0", save->monsDelivered[0]);
    CeSelfTest_PutField("mons1", save->monsDelivered[1]);
    CeSelfTest_PutField("items", save->itemsDelivered);
    CeSelfTest_PutField("party", CalculatePlayerPartyCount());
    CeSelfTest_LineEmit();
}

// Every occupied PC slot (where = box * IN_BOX_COUNT + slot), then the item PC.
static void PutPc(const char *probe)
{
    u32 box, slot, i, mons = 0, items = 0;

    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (slot = 0; slot < IN_BOX_COUNT; slot++)
        {
            u32 species = GetBoxMonDataAt(box, slot, MON_DATA_SPECIES);
            if (species == SPECIES_NONE)
                continue;
            CeSelfTest_LineStart("teamPcMon");
            CeSelfTest_PutField("where", box * IN_BOX_COUNT + slot);
            CeSelfTest_PutField("species", species);
            CeSelfTest_LineEmit();
            mons++;
        }
    }
    for (i = 0; i < PC_ITEMS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->pcItems[i].itemId == ITEM_NONE)
            continue;
        CeSelfTest_LineStart("teamPcItem");
        CeSelfTest_PutField("slot", i);
        CeSelfTest_PutField("item", gSaveBlock1Ptr->pcItems[i].itemId);
        CeSelfTest_PutField("qty", gSaveBlock1Ptr->pcItems[i].quantity);
        CeSelfTest_LineEmit();
        items++;
    }
    CeSelfTest_LineStart(probe);
    CeSelfTest_PutField("mons", mons);
    CeSelfTest_PutField("items", items);
    CeSelfTest_LineEmit();
}

static void NewGame(void)
{
    u32 i, otId = PROBE_OT_ID;

    StringCopy(gSaveBlock2Ptr->playerName, sProbePlayerName);
    gSaveBlock2Ptr->playerGender = MALE;
    for (i = 0; i < TRAINER_ID_LENGTH; i++)
        gSaveBlock2Ptr->playerTrainerId[i] = otId >> (8 * i);
    for (i = 0; i < CE_TEAM_MAX_BADGES; i++)
        FlagClear(FLAG_BADGE01_GET + i);
    ZeroPlayerPartyMons();
    ResetPokemonStorageSystem();
    NewGameInitPCItems();
    CeStart_OnNewGame();
    CeTeam_OnNewGame();
}

static void ProbeNewGame(void)
{
    u32 i, count;

    CeSelfTest_LineStart("teamConfig");
    CeSelfTest_PutField("static", gCeRomConfig.body.team.staticTeam);
    for (count = 0; count < CE_TEAM_MAX_MONS && gCeTeamMons[count].mon.species != SPECIES_NONE; count++)
        ;
    CeSelfTest_PutField("count", count);
    CeSelfTest_PutField("max", CE_TEAM_MAX_MONS);
    CeSelfTest_PutField("boxes", TOTAL_BOXES_COUNT);
    CeSelfTest_PutField("slots", IN_BOX_COUNT);
    CeSelfTest_PutField("otId", PROBE_OT_ID);
    CeSelfTest_LineEmit();

    NewGame();
    PutSave("teamSave");
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) != SPECIES_NONE)
            PutMon(i, &gParties[B_TRAINER_PLAYER][i]);
    }
    for (i = 0; i < count; i++)
    {
        const struct CeTeamMon *entry = &gCeTeamMons[i];
        if (entry->box != CE_TEAM_IN_PARTY && CeTeam_MonDelivered(i))
            PutBoxMon(entry->box, entry->slot);
    }
    PutPc("teamPc");
}

// The badges one at a time, with every delivery the PC makes at each count.
static void ProbeBadges(void)
{
    u32 i, badge, count, timed = 0, blocked = CE_TEAM_MAX_MONS;
    struct Pokemon mon;

    for (count = 0; count < CE_TEAM_MAX_MONS && gCeTeamMons[count].mon.species != SPECIES_NONE; count++)
    {
        if (gCeTeamMons[count].box != CE_TEAM_IN_PARTY && gCeTeamMons[count].badges != 0)
        {
            timed++;
            blocked = count;
        }
    }
    if (timed >= 2)
    {
        const struct CeTeamMon *entry = &gCeTeamMons[blocked];
        CreateMon(&mon, PROBE_PLACEHOLDER, 5, 0, OTID_STRUCT_PRESET(0));
        SetBoxMonAt(entry->box, entry->slot, &mon.box);
        CeSelfTest_LineStart("teamBlock");
        CeSelfTest_PutField("entry", blocked);
        CeSelfTest_PutField("where", entry->box * IN_BOX_COUNT + entry->slot);
        CeSelfTest_PutField("species", PROBE_PLACEHOLDER);
        CeSelfTest_LineEmit();
    }

    for (badge = 0; badge <= CE_TEAM_MAX_BADGES; badge++)
    {
        u32 result, skip = 0;

        if (badge > 0)
            FlagSet(FLAG_BADGE01_GET + badge - 1);
        while ((result = CeTeam_DeliverNext(skip)) != CE_DELIVERY_NONE)
        {
            CeSelfTest_LineStart("teamDelivery");
            CeSelfTest_PutField("badges", CeTeam_BadgeCount());
            CeSelfTest_PutField("result", result);
            if (result == CE_DELIVERY_MON || result == CE_DELIVERY_MON_MOVED)
            {
                CeSelfTest_PutField("where", gSpecialVar_MonBoxId * IN_BOX_COUNT + gSpecialVar_MonBoxPos);
                CeSelfTest_PutField("species", GetBoxMonDataAt(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos, MON_DATA_SPECIES));
            }
            CeSelfTest_LineEmit();
            if (result == CE_DELIVERY_MON || result == CE_DELIVERY_MON_MOVED)
                PutBoxMon(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos);
            if (result == CE_DELIVERY_MON_NO_ROOM || result == CE_DELIVERY_ITEM_NO_ROOM)
                skip++;
        }
        if (badge == 0)
            PutSave("teamSaveBadge0");
    }
    PutSave("teamSaveAll");
    CeSelfTest_LineStart("teamAgain");
    CeSelfTest_PutField("result", CeTeam_DeliverNext(0));
    CeSelfTest_LineEmit();
    PutPc("teamPcAll");
    for (i = 0; i < CE_TEAM_MAX_BADGES; i++)
        FlagClear(FLAG_BADGE01_GET + i);
}

static u32 BlocksCatchingIn(u32 battleTypeFlags)
{
    u32 saved = gBattleTypeFlags, blocks;

    gBattleTypeFlags = battleTypeFlags;
    blocks = CeTeam_BlocksCatching();
    gBattleTypeFlags = saved;
    return blocks;
}

static void ProbeGates(void)
{
    struct CeModesSave *modes = &gSaveBlock2Ptr->pokedex.ceModes;
    struct Pokemon mon;
    u32 noEvolution = modes->rules.noEvolution;
    u32 party = CalculatePlayerPartyCount();
    u32 stone = ITEM_THUNDER_STONE;

    CeSelfTest_LineStart("teamGates");
    CeSelfTest_PutField("refuses", CeTeam_RefusesNewMons());
    CeSelfTest_PutField("wild", BlocksCatchingIn(0));
    CeSelfTest_PutField("scripted", BlocksCatchingIn(BATTLE_TYPE_LEGENDARY));
    CeSelfTest_PutField("tutorial", BlocksCatchingIn(BATTLE_TYPE_CATCH_TUTORIAL));
    CeSelfTest_PutField("trainer", BlocksCatchingIn(BATTLE_TYPE_TRAINER));
    CreateMon(&mon, SPECIES_WURMPLE, 5, 0, OTID_STRUCT_PRESET(0));
    CeSelfTest_PutField("gift", GiveScriptedMonToPlayer(&mon, PARTY_SIZE));
    CeSelfTest_PutField("egg", ScriptGiveEgg(SPECIES_WYNAUT));
    CeSelfTest_PutField("starter", ScriptGiveMon(GetStarterPokemon(0), 5, ITEM_NONE));
    FlagClear(FLAG_PENDING_DAYCARE_EGG);
    TriggerPendingDaycareEgg();
    CeSelfTest_PutField("daycareEgg", FlagGet(FLAG_PENDING_DAYCARE_EGG));
    FlagClear(FLAG_PENDING_DAYCARE_EGG);
    CeSelfTest_PutField("partyBefore", party);
    CeSelfTest_PutField("partyAfter", CalculatePlayerPartyCount());
    CeSelfTest_LineEmit();

    // Evolutions with the run's rule, then with it switched off in the save for a moment.
    CeSelfTest_LineStart("teamEvolution");
    CeSelfTest_PutField("hasRules", CeModes_SaveHasRules());
    CeSelfTest_PutField("noEvolution", CeModes_NoEvolution());
    CeSelfTest_PutField("species", PROBE_EVO_SPECIES);
    CeSelfTest_PutField("level", PROBE_EVO_LEVEL);
    CreateMon(&mon, PROBE_EVO_SPECIES, PROBE_EVO_LEVEL, 0, OTID_STRUCT_PRESET(0));
    CeSelfTest_PutField("levelUp", GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO));
    CreateMon(&mon, SPECIES_PIKACHU, 5, 0, OTID_STRUCT_PRESET(0));
    CeSelfTest_PutField("stoneSpecies", SPECIES_PIKACHU);
    CeSelfTest_PutField("stone", stone);
    CeSelfTest_PutField("stoneUse", GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, stone, NULL, NULL, CHECK_EVO));
    CeSelfTest_PutField("stoneCheck", GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_CHECK, stone, NULL, NULL, CHECK_EVO));
    modes->rules.noEvolution = 0;
    CreateMon(&mon, PROBE_EVO_SPECIES, PROBE_EVO_LEVEL, 0, OTID_STRUCT_PRESET(0));
    CeSelfTest_PutField("levelUpOff", GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO));
    modes->rules.noEvolution = noEvolution;
    CeSelfTest_LineEmit();
}

// Addresses for the headless-mGBA scenario (tools/canary/team.ts); checked by nothing else.
static void ProbeScenario(void)
{
    CeSelfTest_LineStart("teamScenario");
    CeSelfTest_PutField("battleMain", (u32)BattleMainCB2);
    CeSelfTest_PutField("grassGroup", MAP_GROUP(MAP_ROUTE102));
    CeSelfTest_PutField("grassNum", MAP_NUM(MAP_ROUTE102));
    CeSelfTest_PutField("playerGender", offsetof(struct SaveBlock2, playerGender));
    CeSelfTest_LineEmit();
}

void CeSelfTest_ProbeTeam(void)
{
    ProbeScenario();
    ProbeNewGame();
    ProbeBadges();
    ProbeGates();
}
