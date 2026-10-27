#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "move.h"
#include "move_relearner.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokedex_common.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "script.h"
#include "string_util.h"
#include "ce/pokedex.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/relearner.h"
#include "ce/selftest.h"
#include "ce/wonder_trade.h"
#include "config/pokedex_plus_hgss.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/map_groups.h"
#include "constants/moves.h"
#include "constants/pokedex.h"
#include "constants/species.h"
#include "constants/vars.h"

extern struct PokedexView *sPokedexView;

// Self-test for QoL features whose behaviour the on/off masks (ProbeQol in selftest.c) don't
// show. Runs after the player's-choice features were flipped. tools/canary/checks/qol.ts checks
// these lines against the modes the engine wrote. A feature that needs its own lines appends a
// probe function here and calls it from CeSelfTest_ProbeQolExtras.

static const char *const sFeatureFields[] = { "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9" };

// The Pokémon Center 2F sellers: each one's hide flag and feature, which also need Center
// sellers (bits), and which stand upstairs in a Pokémon Center and anywhere else (bits; a
// seller stands while its hide flag is clear).
static void ProbePcSellers(void)
{
    u32 i, count, feature, onPc = 0, elsewhere = 0, center = 0, contiguous = TRUE;
    u16 flag, firstFlag = 0;
    bool32 needsCenterSellers;

    for (count = 0; CeProgression_GetPcSeller(count, &flag, &feature, &needsCenterSellers); count++)
    {
        if (count == 0)
            firstFlag = flag;
        else if (flag != firstFlag + count)
            contiguous = FALSE;
        if (needsCenterSellers)
            center |= 1u << count;
    }

    CeProgression_UpdatePcSellers(TRUE);
    for (i = 0; CeProgression_GetPcSeller(i, &flag, &feature, &needsCenterSellers); i++)
    {
        if (!FlagGet(flag))
            onPc |= 1u << i;
    }
    CeProgression_UpdatePcSellers(FALSE);
    for (i = 0; CeProgression_GetPcSeller(i, &flag, &feature, &needsCenterSellers); i++)
    {
        if (!FlagGet(flag))
            elsewhere |= 1u << i;
    }

    CeSelfTest_LineStart("pcSellers");
    CeSelfTest_PutField("count", count);
    CeSelfTest_PutField("flag0", firstFlag);
    CeSelfTest_PutField("contiguous", contiguous);
    CeSelfTest_PutField("center", center);
    CeSelfTest_PutField("onPc", onPc);
    CeSelfTest_PutField("elsewhere", elsewhere);
    CeSelfTest_PutField("none", CE_QOL_COUNT);
    for (i = 0; i < ARRAY_COUNT(sFeatureFields) && CeProgression_GetPcSeller(i, &flag, &feature, &needsCenterSellers); i++)
        CeSelfTest_PutField(sFeatureFields[i], feature);
    CeSelfTest_LineEmit();
}

// For tools/canary/pc-sellers.ts, which plays a generated ROM in headless mGBA: the maps it
// visits (a standard Pokémon Center 2F, Mauville's with one more NPC, and Brendan's bedroom with
// the old sellers' placeholders), where gObjectEvents lives (to count what spawned), where the
// save keeps the player's own QoL picks (bit = feature id), the flags it clears to spawn
// every object at once, and what it needs to continue a save from an older base made on a 2F
// (the saved object templates, CB2_ContinueSavedGame).
static void ProbeScenario(void)
{
    CeSelfTest_LineStart("scenarioPcSellers");
    CeSelfTest_PutField("oldaleGroup", MAP_GROUP(MAP_OLDALE_TOWN_POKEMON_CENTER_2F));
    CeSelfTest_PutField("oldaleNum", MAP_NUM(MAP_OLDALE_TOWN_POKEMON_CENTER_2F));
    CeSelfTest_PutField("mauvilleGroup", MAP_GROUP(MAP_MAUVILLE_CITY_POKEMON_CENTER_2F));
    CeSelfTest_PutField("mauvilleNum", MAP_NUM(MAP_MAUVILLE_CITY_POKEMON_CENTER_2F));
    CeSelfTest_PutField("brendanGroup", MAP_GROUP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F));
    CeSelfTest_PutField("brendanNum", MAP_NUM(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_2F));
    CeSelfTest_PutField("objectEvents", (u32)gObjectEvents);
    CeSelfTest_PutField("objectEventSize", sizeof(struct ObjectEvent));
    CeSelfTest_PutField("objectEventCount", OBJECT_EVENTS_COUNT);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioPcSellerFlags");
    CeSelfTest_PutField("qolChosen", offsetof(struct SaveBlock2, pokedex.ceQolChosen));
    CeSelfTest_PutField("qolValue", offsetof(struct SaveBlock2, pokedex.ceQolValue));
    CeSelfTest_PutField("zCrystals", CE_QOL_Z_CRYSTALS);
    CeSelfTest_PutField("battleItems", CE_QOL_BATTLE_ITEMS);
    CeSelfTest_PutField("tempB", FLAG_TEMP_B);
    CeSelfTest_PutField("tempC", FLAG_TEMP_C);
    CeSelfTest_PutField("giftMan", FLAG_HIDE_POKEMON_CENTER_2F_MYSTERY_GIFT_MAN);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioContinue");
    CeSelfTest_PutField("continueGame", (u32)CB2_ContinueSavedGame);
    CeSelfTest_PutField("objTemplates", offsetof(struct SaveBlock1, objectEventTemplates));
    CeSelfTest_PutField("templateSize", sizeof(struct ObjectEventTemplate));
    CeSelfTest_PutField("templateCount", OBJECT_EVENT_TEMPLATES_COUNT);
    CeSelfTest_PutField("objLocalId", offsetof(struct ObjectEvent, localId));
    CeSelfTest_PutField("followerId", OBJ_EVENT_ID_FOLLOWER);
    CeSelfTest_LineEmit();
}

// Poké Balls always catch: the hook in SetBallThrowShakes (battle_script_commands.c) reads
// CeQol_IsOn(CE_QOL_ALWAYS_CATCH) when a ball is thrown, so the game must see the mode the website
// wrote, after the player's pick. The catch itself is covered by test/ce/battles.c.
static void ProbeAlwaysCatch(void)
{
    CeSelfTest_LineStart("alwaysCatch");
    CeSelfTest_PutField("id", CE_QOL_ALWAYS_CATCH);
    CeSelfTest_PutField("mode", CeQol_GetMode(CE_QOL_ALWAYS_CATCH));
    CeSelfTest_PutField("on", CeQol_IsOn(CE_QOL_ALWAYS_CATCH));
    CeSelfTest_LineEmit();
}

// Where struct Sprite keeps its inUse bit (a bitfield): the byte and the mask, so the canary can
// count the sprites alive on each dex page.
static u32 SpriteInUseByte(u32 *mask)
{
    struct Sprite sprite;
    const u8 *bytes = (const u8 *)&sprite;
    u32 i;

    memset(&sprite, 0, sizeof(sprite));
    sprite.inUse = TRUE;
    for (i = 0; i < sizeof(sprite); i++)
    {
        if (bytes[i] != 0)
        {
            *mask = bytes[i];
            return i;
        }
    }
    *mask = 0;
    return 0;
}

// Pokédex Plus: the dex reads POKEDEX_PLUS_HGSS (include/config/pokedex_plus_hgss.h) whenever a
// screen starts, so it must follow the mode the website wrote, after the player's pick. The
// scenarioDex lines give tools/canary/pokedex.ts, which plays the HGSS dex in headless mGBA, the
// save fields it sets (seen and caught flags, the national dex, the list's mode and order) and the
// dex's code and state. The spoiler and bounds helpers are covered by test/ce/pokedex.c.
static void ProbePokedexPlus(void)
{
    u32 inUseMask;

    CeSelfTest_LineStart("pokedexPlus");
    CeSelfTest_PutField("id", CE_QOL_POKEDEX_PLUS);
    CeSelfTest_PutField("mode", CeQol_GetMode(CE_QOL_POKEDEX_PLUS));
    CeSelfTest_PutField("on", CeQol_IsOn(CE_QOL_POKEDEX_PLUS));
    CeSelfTest_PutField("hgss", POKEDEX_PLUS_HGSS);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioDex");
    CeSelfTest_PutField("dexSeen", offsetof(struct SaveBlock1, dexSeen));
    CeSelfTest_PutField("dexCaught", offsetof(struct SaveBlock1, dexCaught));
    CeSelfTest_PutField("dexFlagBytes", NUM_DEX_FLAG_BYTES);
    CeSelfTest_PutField("dexGet", FLAG_SYS_POKEDEX_GET);
    CeSelfTest_PutField("nationalFlag", FLAG_SYS_NATIONAL_DEX);
    CeSelfTest_PutField("nationalVar", VAR_NATIONAL_DEX - VARS_START);
    CeSelfTest_PutField("vars", offsetof(struct SaveBlock1, vars));
    CeSelfTest_PutField("nationalMagic", offsetof(struct SaveBlock2, pokedex.nationalMagic));
    CeSelfTest_PutField("dexMode", offsetof(struct SaveBlock2, pokedex.mode));
    CeSelfTest_PutField("dexOrder", offsetof(struct SaveBlock2, pokedex.order));
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioDexCode");
    CeSelfTest_PutField("modeNational", DEX_MODE_NATIONAL);
    CeSelfTest_PutField("orderAlphabetical", ORDER_ALPHABETICAL);
    CeSelfTest_PutField("pokedexView", (u32)&sPokedexView);
    CeSelfTest_PutField("openDex", (u32)CB2_OpenPokedex);
    CeSelfTest_PutField("sprites", (u32)gSprites);
    CeSelfTest_PutField("spriteSize", sizeof(struct Sprite));
    CeSelfTest_PutField("spriteCount", MAX_SPRITES);
    CeSelfTest_PutField("inUseByte", SpriteInUseByte(&inUseMask));
    CeSelfTest_PutField("inUseMask", inUseMask);
    CeSelfTest_PutField("qolPokedexPlus", CE_QOL_POKEDEX_PLUS);
    CeSelfTest_LineEmit();

    // Fields of the dex's view: the list's Pokémon sprites (whose x tells the HGSS list from the
    // classic one) and the Evos page's pre-evolutions and rows.
    CeSelfTest_LineStart("scenarioDexView");
    CeSelfTest_PutField("viewMonSprites", offsetof(struct PokedexView, monSpriteIds));
    CeSelfTest_PutField("viewMonSpriteCount", MAX_MONS_ON_SCREEN);
    CeSelfTest_PutField("spriteX", offsetof(struct Sprite, x));
    CeSelfTest_PutField("viewPreEvos", offsetof(struct PokedexView, numPreEvolutions));
    CeSelfTest_PutField("viewEvoTargets", offsetof(struct PokedexView, sEvoScreenData.targetSpecies));
    CeSelfTest_PutField("evoTargetSize", sizeof(sPokedexView->sEvoScreenData.targetSpecies[0]));
    CeSelfTest_LineEmit();
}

// Egg and tutor moves at the Move Relearner: the Fallarbor relearner and the key item run
// CeRelearner_ScriptStartList each time, which reads CeQol_IsOn(CE_QOL_RELEARNER_EGG_TUTOR), so
// the game must see the mode the website wrote, after the player's pick. asks: whether it then
// asks which list (VAR_RESULT); state: the list it starts from (the level-up one). The Pokémon
// Center move tutor (CE_QOL_TUTOR_SELLER) is the FLAG_TEMP_B seller of the pcSellers line; the
// lists themselves are the learnsets probe's relearn lines.
static void ProbeRelearner(void)
{
    struct ScriptContext ctx;

    memset(&ctx, 0, sizeof(ctx));
    gMoveRelearnerState = MOVE_RELEARNER_TUTOR_MOVES;
    CeRelearner_ScriptStartList(&ctx);
    CeSelfTest_LineStart("relearnerEggTutor");
    CeSelfTest_PutField("id", CE_QOL_RELEARNER_EGG_TUTOR);
    CeSelfTest_PutField("mode", CeQol_GetMode(CE_QOL_RELEARNER_EGG_TUTOR));
    CeSelfTest_PutField("on", CeQol_IsOn(CE_QOL_RELEARNER_EGG_TUTOR));
    CeSelfTest_PutField("asks", gSpecialVar_Result);
    CeSelfTest_PutField("state", gMoveRelearnerState);
    CeSelfTest_LineEmit();
}

// For tools/canary/relearner.ts, which plays both relearners in headless mGBA: the Fallarbor
// relearner's house, where the game keeps the relearner's list (to read which list is open),
// the list states, and an Azumarill (personality and OT id 0, so it isn't encrypted and is valid
// in any run) that knows four moves, so teaching one opens the forget-a-move screen. The canary
// puts it in the party and in the first slot of the first PC box, where the relearner's PC view
// opens.
#define SCENARIO_MON_WORDS_PER_LINE 8

static void ProbeRelearnerScenario(void)
{
    static const char *const names[SCENARIO_MON_WORDS_PER_LINE] = { "w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7" };
    static const u16 sMoves[MAX_MON_MOVES] = { MOVE_TACKLE, MOVE_WATER_GUN, MOVE_DEFENSE_CURL, MOVE_TAIL_WHIP };
    struct Pokemon mon;
    const u32 *words = (const u32 *)&mon;
    u32 i, j;

    CeSelfTest_LineStart("scenarioRelearner");
    CeSelfTest_PutField("fallarborGroup", MAP_GROUP(MAP_FALLARBOR_TOWN_MOVE_RELEARNERS_HOUSE));
    CeSelfTest_PutField("fallarborNum", MAP_NUM(MAP_FALLARBOR_TOWN_MOVE_RELEARNERS_HOUSE));
    CeSelfTest_PutField("relearnerState", (u32)&gMoveRelearnerState);
    CeSelfTest_PutField("levelUpList", MOVE_RELEARNER_LEVEL_UP_MOVES);
    CeSelfTest_PutField("eggList", MOVE_RELEARNER_EGG_MOVES);
    CeSelfTest_PutField("tutorList", MOVE_RELEARNER_TUTOR_MOVES);
    CeSelfTest_PutField("eggTutor", CE_QOL_RELEARNER_EGG_TUTOR);
    CeSelfTest_PutField("tutorSeller", CE_QOL_TUTOR_SELLER);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioRelearnerBox");
    CeSelfTest_PutField("storagePtr", (u32)&gPokemonStoragePtr);
    CeSelfTest_PutField("boxes", offsetof(struct PokemonStorage, boxes));
    CeSelfTest_PutField("boxMonSize", sizeof(struct BoxPokemon));
    CeSelfTest_LineEmit();

    CreateMon(&mon, SPECIES_AZUMARILL, 50, 0, OTID_STRUCT_PRESET(0));
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u8 pp = GetMovePP(sMoves[i]);
        SetMonData(&mon, MON_DATA_MOVE1 + i, &sMoves[i]);
        SetMonData(&mon, MON_DATA_PP1 + i, &pp);
    }
    CalculateMonStats(&mon);
    for (i = 0; i < sizeof(mon) / 4; i += SCENARIO_MON_WORDS_PER_LINE)
    {
        CeSelfTest_LineStart("scenarioRelearnerMon");
        CeSelfTest_PutField("i", i);
        for (j = 0; j < SCENARIO_MON_WORDS_PER_LINE && i + j < sizeof(mon) / 4; j++)
            CeSelfTest_PutField(names[j], words[i + j]);
        CeSelfTest_LineEmit();
    }
}

// Wonder Trade: the pool the attendant draws from, read through the game's own rule, so the canary
// can compare it with the engine's (the battle-only list must match BATTLE_ONLY_FORMS, and the
// Pokédex entries in the pool, for any type and for the run's, the engine's prediction). spots:
// bit i = sWonderTradeSpots[i] is in the pool (any type). picks: of 20 picks for the run's type,
// how many were in that pool. block and allowed: the gate and the attendant's visibility with
// the ROM's own rules, after the player's pick. The trade itself is covered by
// test/ce/wonder_trade.c and played by tools/canary/wonder-trade.ts.
static const u16 sWonderTradeSpots[] =
{
    SPECIES_VENUSAUR_MEGA, SPECIES_KYOGRE_PRIMAL, SPECIES_MIMIKYU_BUSTED, SPECIES_ARCEUS_FIRE,
    SPECIES_KYUREM_WHITE, SPECIES_ROTOM_WASH, SPECIES_DARMANITAN_GALAR_ZEN,
    SPECIES_MEWTWO, SPECIES_VULPIX_ALOLA, SPECIES_DARMANITAN_GALAR_STANDARD, SPECIES_ARCEUS_NORMAL, SPECIES_ROTOM,
};

static void ProbeWonderTrade(void)
{
    u32 i, sum = 0, spots = 0, picks = 0, type = CeModes_Rules()->monotype;

    for (i = 0; i < CeWonderTrade_BattleOnlyCount(); i++)
        sum += CeWonderTrade_BattleOnlyForm(i);
    for (i = 0; i < ARRAY_COUNT(sWonderTradeSpots); i++)
    {
        if (CeWonderTrade_InPool(sWonderTradeSpots[i], TYPE_NONE))
            spots |= 1u << i;
    }
    for (i = 0; i < 20; i++)
    {
        if (CeWonderTrade_InPool(CeWonderTrade_PickSpecies(type), type))
            picks++;
    }

    CeSelfTest_LineStart("wonderTrade");
    CeSelfTest_PutField("id", CE_QOL_WONDER_TRADE);
    CeSelfTest_PutField("mode", CeQol_GetMode(CE_QOL_WONDER_TRADE));
    CeSelfTest_PutField("on", CeQol_IsOn(CE_QOL_WONDER_TRADE));
    CeSelfTest_PutField("block", CeWonderTrade_Block());
    CeSelfTest_PutField("allowed", CeWonderTrade_IsAllowed());
    CeSelfTest_PutField("battleOnly", CeWonderTrade_BattleOnlyCount());
    CeSelfTest_PutField("battleOnlySum", sum);
    CeSelfTest_PutField("groups", CeWonderTrade_CountGroups(TYPE_NONE));
    CeSelfTest_PutField("runType", type);
    CeSelfTest_PutField("runGroups", CeWonderTrade_CountGroups(type));
    CeSelfTest_PutField("spots", spots);
    CeSelfTest_PutField("picks", picks);
    CeSelfTest_LineEmit();
}

// For tools/canary/wonder-trade.ts, which plays Wonder Trades in headless mGBA: the special flag
// set while the scene plays (its byte and bit), the string buffers the attendant fills (the
// received Pokémon's name and OT, and the box), where a party Pokémon keeps its level and HP and
// a box Pokémon its personality and OT id (unencrypted fields), and where the save keeps the
// player's trainer id.
static void ProbeWonderTradeScenario(void)
{
    CeSelfTest_LineStart("scenarioWonderTrade");
    CeSelfTest_PutField("wonderTrade", CE_QOL_WONDER_TRADE);
    CeSelfTest_PutField("tradeFlag", FLAG_CE_WONDER_TRADE);
    CeSelfTest_PutField("tradeFlagByte", (u32)GetFlagPointer(FLAG_CE_WONDER_TRADE));
    CeSelfTest_PutField("tradeFlagMask", 1u << (FLAG_CE_WONDER_TRADE & 7));
    CeSelfTest_PutField("stringVar1", (u32)gStringVar1);
    CeSelfTest_PutField("stringVar2", (u32)gStringVar2);
    CeSelfTest_PutField("stringVar3", (u32)gStringVar3);
    CeSelfTest_LineEmit();

    CeSelfTest_LineStart("scenarioWonderTradeMon");
    CeSelfTest_PutField("monLevel", offsetof(struct Pokemon, level));
    CeSelfTest_PutField("monHp", offsetof(struct Pokemon, hp));
    CeSelfTest_PutField("monMaxHp", offsetof(struct Pokemon, maxHP));
    CeSelfTest_PutField("personality", offsetof(struct BoxPokemon, personality));
    CeSelfTest_PutField("otId", offsetof(struct BoxPokemon, otId));
    CeSelfTest_PutField("playerId", offsetof(struct SaveBlock2, playerTrainerId));
    CeSelfTest_LineEmit();
}

void CeSelfTest_ProbeQolExtras(void)
{
    ProbePcSellers();
    ProbeScenario();
    ProbeAlwaysCatch();
    ProbePokedexPlus();
    ProbeRelearner();
    ProbeRelearnerScenario();
    ProbeWonderTrade();
    ProbeWonderTradeScenario();
}
