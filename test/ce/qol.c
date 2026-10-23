#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_main.h"
#include "battle_util.h"
#include "event_data.h"
#include "item.h"
#include "item_use.h"
#include "malloc.h"
#include "pokemon.h"
#include "string_util.h"
#include "test/test.h"
#include "ce/progression.h"
#include "ce/qol.h"
#include "ce/speed.h"
#include "ce/test.h"
#include "constants/battle.h"

// QoL gates (ce/qol.h): each feature is checked where upstream reads its config.

TEST("CE: Player's-choice QoL modes start from the website's pick and follow the player's")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    bool32 picked = FALSE, pick = FALSE, expected = FALSE;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF;        picked = TRUE;  pick = TRUE;  expected = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;         picked = TRUE;  pick = FALSE; expected = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_OFF; picked = FALSE;               expected = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_ON;  picked = FALSE;               expected = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_OFF; picked = TRUE;  pick = TRUE;  expected = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_PLAYER_ON;  picked = TRUE;  pick = FALSE; expected = FALSE; }

    gSaveBlock2Ptr->pokedex.ceQolChosen[0] = gSaveBlock2Ptr->pokedex.ceQolValue[0] = 0;
    CeTest_SetQol(CE_QOL_REUSABLE_TMS, mode);
    if (picked)
        CeQol_SetPlayerValue(CE_QOL_REUSABLE_TMS, pick);
    EXPECT_EQ(CeQol_IsOn(CE_QOL_REUSABLE_TMS), expected);
}

TEST("CE: Reusable TMs are not used up when taught")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    ASSUME(GetItemPocket(ITEM_TM01) == POCKET_TM_HM);
    ASSUME(gItemsInfo[ITEM_TM01].importance == 0);
    CeTest_SetQol(CE_QOL_REUSABLE_TMS, mode);
    // The party menu removes a taught TM from the bag only if it isn't important.
    EXPECT_EQ(GetItemImportance(ITEM_TM01) != 0, mode == CE_QOL_MODE_ON);
    EXPECT_NE(GetItemImportance(ITEM_HM01), 0);
    EXPECT_EQ(GetItemImportance(ITEM_POTION), 0);
}

TEST("CE: Party Exp. Share needs no key item or flag")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    CeTest_SetQol(CE_QOL_EXP_SHARE_ALL, mode);
    EXPECT_EQ(IsGen6ExpShareEnabled(), mode == CE_QOL_MODE_ON);
}

// A level 100 Pokémon from another trainer and no badges: vanilla obeys about one time in ten.
TEST("CE: Always obey makes traded Pokémon obey without badges")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    static const u8 sOtherOt[] = _("OTHER");
    u32 i, obeyed = 0;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    CeTest_SetQol(CE_QOL_ALWAYS_OBEY, mode);
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gBattleTypeFlags = 0;
    gBattlerAttacker = 0;
    gBattlerBattleController[0] = BATTLE_CONTROLLER_PLAYER;
    memset(&gBattleMons[0], 0, sizeof(gBattleMons[0]));
    gBattleMons[0].species = SPECIES_WOBBUFFET;
    gBattleMons[0].level = 100;
    gBattleMons[0].otId = 12345;
    StringCopy(gBattleMons[0].otName, sOtherOt);
    gCurrentMove = MOVE_CELEBRATE;
    for (i = 0; i < 32; i++)
    {
        if (GetAttackerObedienceForAction() == OBEYS)
            obeyed++;
    }
    FREE_AND_SET_NULL(gBattleStruct);
    if (mode == CE_QOL_MODE_ON)
        EXPECT_EQ(obeyed, 32);
    else
        EXPECT_LT(obeyed, 32);
}

TEST("CE: Trade evolution items work from the bag without trading")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    struct Pokemon mon;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON; }

    CeTest_SetQol(CE_QOL_TRADE_EVOS_WITHOUT_TRADE, mode);
    if (mode == CE_QOL_MODE_ON)
    {
        EXPECT_EQ(GetItemType(ITEM_METAL_COAT), ITEM_USE_PARTY_MENU);
        EXPECT(GetItemFieldFunc(ITEM_METAL_COAT) == ItemUseOutOfBattle_EvolutionStone);
    }
    else
    {
        EXPECT_EQ(GetItemType(ITEM_METAL_COAT), gItemsInfo[ITEM_METAL_COAT].type);
        EXPECT(GetItemFieldFunc(ITEM_METAL_COAT) == gItemsInfo[ITEM_METAL_COAT].fieldUseFunc);
    }
    // Razor Claw's evolution needs no trade, so it stays a held item.
    EXPECT_EQ(GetItemType(ITEM_RAZOR_CLAW), gItemsInfo[ITEM_RAZOR_CLAW].type);
    // Used from the bag like a stone, the item evolves Onix.
    CreateMon(&mon, SPECIES_ONIX, 30, 0, OTID_STRUCT_PLAYER_ID);
    EXPECT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_ITEM_USE, ITEM_METAL_COAT, NULL, NULL, CHECK_EVO), SPECIES_STEELIX);
}

TEST("CE: Forfeiting trainer battles follows the QoL, never in link battles")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u32 flags = 0;
    bool32 expected = FALSE;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; flags = BATTLE_TYPE_TRAINER;                    expected = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  flags = BATTLE_TYPE_TRAINER;                    expected = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK; expected = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER; expected = FALSE; }

    CeTest_SetQol(CE_QOL_FORFEIT_TRAINER_BATTLES, mode);
    gBattleTypeFlags = flags;
    EXPECT_EQ(CanPlayerForfeitNormalTrainerBattle() != 0, expected);
    gBattleTypeFlags = 0;
}

TEST("CE: Fast HP bar and short pauses follow the QoL, never in link battles")
{
    enum CeQolMode mode = CE_QOL_MODE_OFF;
    u32 flags = 0;
    bool32 expected = FALSE;

    PARAMETRIZE { mode = CE_QOL_MODE_OFF; flags = 0;                expected = FALSE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  flags = 0;                expected = TRUE; }
    PARAMETRIZE { mode = CE_QOL_MODE_ON;  flags = BATTLE_TYPE_LINK; expected = FALSE; }

    CeTest_SetQol(CE_QOL_FAST_HP_BAR, mode);
    CeTest_SetQol(CE_QOL_SHORT_BATTLE_PAUSES, mode);
    gBattleTypeFlags = flags;
    EXPECT_EQ(B_FAST_HP_DRAIN, expected);
    EXPECT_EQ(CeSpeed_BattleWait(64), expected ? 32 : 64);
    gBattleTypeFlags = 0;
}
