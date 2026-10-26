#include "global.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/selftest.h"
#include "constants/flags.h"
#include "constants/map_groups.h"

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

void CeSelfTest_ProbeQolExtras(void)
{
    ProbePcSellers();
    ProbeScenario();
}
