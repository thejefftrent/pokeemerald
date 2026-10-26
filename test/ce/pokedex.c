#include "global.h"
#include "pokedex.h"
#include "pokedex_plus_hgss.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "strings.h"
#include "task.h"
#include "test/test.h"
#include "ce/pokedex.h"
#include "ce/qol.h"
#include "ce/test.h"
#include "config/pokedex_plus_hgss.h"
#include "constants/pokedex.h"

// Pokédex Plus (CE_QOL_POKEDEX_PLUS, ce/pokedex.h). The HGSS screens themselves, and the
// caught-Pokémon page in a wild battle, are played by tools/canary/pokedex.ts: the test runner's
// recorded battles skip the Pokédex page.

TEST("CE: Pokédex Plus switches expansion's HGSS Pokédex at runtime")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_ON; }

    gSaveBlock2Ptr->pokedex.ceQolChosen[1] = gSaveBlock2Ptr->pokedex.ceQolValue[1] = 0;
    CeTest_SetQol(CE_QOL_POKEDEX_PLUS, mode);
    EXPECT_EQ(POKEDEX_PLUS_HGSS, (mode == CE_QOL_MODE_ON || mode == CE_QOL_MODE_PLAYER_ON));
    EXPECT_EQ(POKEDEX_PLUS_HGSS, CeQol_IsOn(CE_QOL_POKEDEX_PLUS));
}

TEST("CE: Pokédex Plus follows the player's pick")
{
    bool32 pick = FALSE;

    PARAMETRIZE { pick = FALSE; }
    PARAMETRIZE { pick = TRUE; }

    gSaveBlock2Ptr->pokedex.ceQolChosen[1] = gSaveBlock2Ptr->pokedex.ceQolValue[1] = 0;
    CeTest_SetQol(CE_QOL_POKEDEX_PLUS, pick ? CE_QOL_MODE_PLAYER_OFF : CE_QOL_MODE_PLAYER_ON);
    CeQol_SetPlayerValue(CE_QOL_POKEDEX_PLUS, pick);
    EXPECT_EQ(POKEDEX_PLUS_HGSS, pick);
}

TEST("CE: Pokédex Plus: the caught-Pokémon page is the HGSS one only while it is on")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u8 taskId;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    CeTest_SetQol(CE_QOL_POKEDEX_PLUS, mode);
    taskId = DisplayCaughtMonDexPage(SPECIES_BULBASAUR, FALSE, 0);
    EXPECT(gTasks[taskId].isActive);
    EXPECT_EQ(gTasks[taskId].func == Task_DisplayCaughtMonDexPageHGSS, mode == CE_QOL_MODE_ON);
    DestroyTask(taskId);
}

TEST("CE: Pokédex Plus: unseen species show as ??? with the question-mark icon")
{
    GetSetPokedexFlag(NATIONAL_DEX_BULBASAUR, FLAG_SET_SEEN);

    EXPECT(CeDex_IsSeen(SPECIES_BULBASAUR));
    EXPECT_EQ(CeDex_SeenOrUnknown(SPECIES_BULBASAUR), SPECIES_BULBASAUR);
    EXPECT(CeDex_SeenName(SPECIES_BULBASAUR) == GetSpeciesName(SPECIES_BULBASAUR));

    EXPECT(!CeDex_IsSeen(SPECIES_IVYSAUR));
    EXPECT_EQ(CeDex_SeenOrUnknown(SPECIES_IVYSAUR), SPECIES_NONE);
    EXPECT(CeDex_SeenName(SPECIES_IVYSAUR) == gText_ThreeQuestionMarks);

    // Caught without the seen flag still reads as unseen: the dex sets both together.
    GetSetPokedexFlag(NATIONAL_DEX_VENUSAUR, FLAG_SET_CAUGHT);
    EXPECT(!CeDex_IsSeen(SPECIES_VENUSAUR));

    // SPECIES_NONE has its own icon (the question mark), not another species'.
    EXPECT(GetMonIconTiles(SPECIES_NONE, 0) == gSpeciesInfo[SPECIES_NONE].iconSprite);
    EXPECT(GetMonIconTiles(SPECIES_NONE, 0) != GetMonIconTiles(SPECIES_IVYSAUR, 0));
}

TEST("CE: Pokédex Plus: forms share their species' seen flag")
{
    ASSUME(SpeciesToNationalPokedexNum(SPECIES_RATTATA_ALOLA) == NATIONAL_DEX_RATTATA);
    EXPECT(!CeDex_IsSeen(SPECIES_RATTATA_ALOLA));
    GetSetPokedexFlag(NATIONAL_DEX_RATTATA, FLAG_SET_SEEN);
    EXPECT(CeDex_IsSeen(SPECIES_RATTATA_ALOLA));
    EXPECT(CeDex_SeenName(SPECIES_RATTATA_ALOLA) == GetSpeciesName(SPECIES_RATTATA_ALOLA));
}

TEST("CE: Pokédex Plus: no species and out-of-range ids are never seen")
{
    // Every flag set: the checks must come from the ids, not from the flags.
    memset(gSaveBlock1Ptr->dexSeen, 0xFF, sizeof(gSaveBlock1Ptr->dexSeen));

    EXPECT(!CeDex_IsSeen(SPECIES_NONE));
    EXPECT(!CeDex_IsSeen(NUM_SPECIES));
    EXPECT(!CeDex_IsSeen(NUM_SPECIES + 5));
    EXPECT(!CeDex_IsSeen(0xFFFF));
    EXPECT_EQ(CeDex_SeenOrUnknown(NUM_SPECIES + 5), SPECIES_NONE);
    EXPECT(CeDex_SeenName(NUM_SPECIES + 5) == gText_ThreeQuestionMarks);
    EXPECT(CeDex_SeenName(SPECIES_NONE) == gText_ThreeQuestionMarks);
    EXPECT(CeDex_IsSeen(SPECIES_BULBASAUR));
}
