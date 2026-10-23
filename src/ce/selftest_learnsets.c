#include "global.h"
#include "item.h"
#include "party_menu.h"
#include "pokemon.h"
#include "ce/rom_config.h"
#include "ce/selftest.h"
#include "ce/teachable.h"
#include "constants/items.h"
#include "constants/moves.h"

// Learnsets, TM/HM/tutor compatibility and evolutions, as game code sees them, for every
// species in the self-test list (gCeRomConfig.body.selfTest.species):
//   tm:      the move each TM/HM item teaches (ItemIdToBattleMoveId, as the bag and party menu use)
//   teach:   bit n = CanLearnTeachableMove(species, move of teachable slot n)
//   levelUp: the moves a new mon gets at a few levels (CreateMon + GiveMonInitialMoveset)
//   evo:     GetEvolutionTargetSpecies for each of the first evolutions, set up to meet its method

#define TM_MOVES_PER_LINE 8
#define EVOS_PROBED 4
#define EVO_NOT_PROBED 0xFFFF

static const u8 sLevels[] = { 1, 15, 40, MAX_LEVEL };

static void ProbeTmMoves(void)
{
    static const char *const names[TM_MOVES_PER_LINE] = { "m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7" };
    u32 first, i;

    for (first = 0; first < NUM_ALL_MACHINES; first += TM_MOVES_PER_LINE)
    {
        CeSelfTest_LineStart("tm");
        CeSelfTest_PutField("first", first);
        for (i = 0; i < TM_MOVES_PER_LINE && first + i < NUM_ALL_MACHINES; i++)
            CeSelfTest_PutField(names[i], ItemIdToBattleMoveId(GetTMHMItemId(first + i + 1)));
        CeSelfTest_LineEmit();
    }
}

static void ProbeTeachable(u16 species)
{
    static const char *const names[CE_TEACHABLE_SLOTS / 32] = { "w0", "w1", "w2", "w3" };
    u32 word, bit;

    CeSelfTest_LineStart("teach");
    CeSelfTest_PutField("species", species);
    for (word = 0; word < CE_TEACHABLE_SLOTS / 32; word++)
    {
        u32 mask = 0;
        for (bit = 0; bit < 32; bit++)
        {
            u32 move = CeTeachable_SlotMove(word * 32 + bit);
            if (move != MOVE_NONE && CanLearnTeachableMove(species, move))
                mask |= 1u << bit;
        }
        CeSelfTest_PutField(names[word], mask);
    }
    CeSelfTest_LineEmit();
}

static void ProbeLevelUp(u16 species)
{
    static const char *const names[MAX_MON_MOVES] = { "m0", "m1", "m2", "m3" };
    struct Pokemon mon;
    u32 i, j;

    for (i = 0; i < ARRAY_COUNT(sLevels); i++)
    {
        CreateMon(&mon, species, sLevels[i], 0, OTID_STRUCT_PRESET(0));
        GiveMonInitialMoveset(&mon);
        CeSelfTest_LineStart("levelUp");
        CeSelfTest_PutField("species", species);
        CeSelfTest_PutField("level", sLevels[i]);
        for (j = 0; j < MAX_MON_MOVES; j++)
            CeSelfTest_PutField(names[j], GetMonData(&mon, MON_DATA_MOVE1 + j));
        CeSelfTest_LineEmit();
    }
}

// Each evolution is tried with a fresh mon that meets its method: EVO_LEVEL at exactly that
// level, EVO_ITEM by using that item at level 50, EVO_TRADE by trading at level 50.
static void ProbeEvolutions(u16 species)
{
    static const char *const names[EVOS_PROBED] = { "e0", "e1", "e2", "e3" };
    const struct Evolution *evolutions = GetSpeciesEvolutions(species);
    struct Pokemon mon;
    u32 count = 0, i;

    while (evolutions != NULL && evolutions[count].method != EVOLUTIONS_END)
        count++;

    CeSelfTest_LineStart("evo");
    CeSelfTest_PutField("species", species);
    CeSelfTest_PutField("n", count);
    for (i = 0; i < EVOS_PROBED; i++)
    {
        u32 result = EVO_NOT_PROBED;
        if (i < count)
        {
            u32 param = evolutions[i].param;
            switch (evolutions[i].method)
            {
            case EVO_LEVEL:
                CreateMon(&mon, species, param == 0 ? 1 : min(param, MAX_LEVEL), 0, OTID_STRUCT_PRESET(0));
                result = GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, ITEM_NONE, NULL, NULL, CHECK_EVO);
                break;
            case EVO_ITEM:
                CreateMon(&mon, species, 50, 0, OTID_STRUCT_PRESET(0));
                result = GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, param, NULL, NULL, CHECK_EVO);
                break;
            case EVO_TRADE:
                CreateMon(&mon, species, 50, 0, OTID_STRUCT_PRESET(0));
                result = GetEvolutionTargetSpecies(&mon, EVO_MODE_TRADE, ITEM_NONE, NULL, NULL, CHECK_EVO);
                break;
            }
        }
        CeSelfTest_PutField(names[i], result);
    }
    CeSelfTest_LineEmit();
}

void CeSelfTest_ProbeLearnsets(void)
{
    const struct CeSelfTestConfig *config = &gCeRomConfig.body.selfTest;
    u32 i;

    ProbeTmMoves();
    for (i = 0; i < CE_SELF_TEST_SPECIES && config->species[i] != SPECIES_NONE; i++)
    {
        ProbeTeachable(config->species[i]);
        ProbeLevelUp(config->species[i]);
        ProbeEvolutions(config->species[i]);
    }
}
