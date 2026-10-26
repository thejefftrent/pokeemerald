#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "pokedex.h"
#include "pokedex_common.h"
#include "ce/pokedex.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/selftest.h"
#include "config/pokedex_plus_hgss.h"
#include "constants/flags.h"
#include "constants/map_groups.h"
#include "constants/pokedex.h"
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
// save keeps the player's own QoL picks (bit = feature id), and the flags it clears to spawn
// every object at once.
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
}

void CeSelfTest_ProbeQolExtras(void)
{
    ProbePcSellers();
    ProbeScenario();
    ProbeAlwaysCatch();
    ProbePokedexPlus();
}
