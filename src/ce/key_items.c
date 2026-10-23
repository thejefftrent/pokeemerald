#include "global.h"
#include "battle_pyramid.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "item.h"
#include "item_menu.h"
#include "item_use.h"
#include "menu.h"
#include "overworld.h"
#include "pokemon.h"
#include "script.h"
#include "sound.h"
#include "task.h"
#include "ce/key_items.h"
#include "ce/qol.h"
#include "constants/item.h"
#include "constants/items.h"
#include "constants/map_types.h"
#include "constants/songs.h"

struct CeKeyItem
{
    u16 item;
    u8 feature; // enum CeQolFeature
};

// When the Key Items pocket is full, CeKeyItems_MakeRoomFor puts these away from the end.
static const struct CeKeyItem sCeKeyItems[] =
{
    { ITEM_CE_CANDY_JAR,      CE_QOL_CANDY_JAR },
    { ITEM_POKEMON_BOX_LINK,  CE_QOL_PC_ITEM },
    { ITEM_CE_MOVE_RELEARNER, CE_QOL_RELEARNER_ITEM },
    { ITEM_CE_MOVE_DELETER,   CE_QOL_DELETER_ITEM },
    { ITEM_CE_INFINITE_REPEL, CE_QOL_INFINITE_REPEL },
};

// item.c sends every bag call to the Pyramid's own bag in this state.
static bool32 IsPyramidBagActive(void)
{
    return CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || FlagGet(FLAG_STORING_ITEMS_IN_PYRAMID_BAG);
}

static bool32 IsCeKeyItem(u16 item)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sCeKeyItems); i++)
    {
        if (sCeKeyItems[i].item == item)
            return TRUE;
    }
    return FALSE;
}

static void TakeKeyItem(u16 item)
{
    RemoveBagItem(item, CountTotalItemQuantityInBag(item));
    if (gSaveBlock1Ptr->registeredItem == item)
        gSaveBlock1Ptr->registeredItem = ITEM_NONE;
}

void CeKeyItems_Sync(void)
{
    u32 i;

    if (IsPyramidBagActive())
        return;

    for (i = 0; i < ARRAY_COUNT(sCeKeyItems); i++)
    {
        u16 item = sCeKeyItems[i].item;
        bool32 has = CheckBagHasItem(item, 1);

        if (CeQol_IsOn(sCeKeyItems[i].feature))
        {
            // Fails only when the pocket is full; the next call tries again.
            if (!has)
                AddBagItem(item, 1);
        }
        else if (has)
        {
            TakeKeyItem(item);
        }
    }

    if (!CeQol_IsOn(CE_QOL_INFINITE_REPEL) && CeKeyItems_IsInfiniteRepelOn())
        VarSet(VAR_REPEL_STEP_COUNT, 0);
}

// Called from inside AddBagItem/CheckBagHasSpace, after their Pyramid redirect, so the
// bag calls here reach the normal bag. The items put away are never the one being added.
void CeKeyItems_MakeRoomFor(u16 itemId, u16 count)
{
    s32 i;

    if (GetItemPocket(itemId) != POCKET_KEY_ITEMS || IsCeKeyItem(itemId))
        return;
    for (i = ARRAY_COUNT(sCeKeyItems) - 1; i >= 0 && GetFreeSpaceForItemInBag(itemId) < count; i--)
    {
        if (CheckBagHasItem(sCeKeyItems[i].item, 1))
            TakeKeyItem(sCeKeyItems[i].item);
    }
}

#define tUsingRegisteredKeyItem data[3] // set by UseRegisteredKeyItemOnField

bool32 CeKeyItems_BlockPartyItemUse(u8 taskId)
{
    if (CalculatePlayerPartyCount() != 0)
        return FALSE;
    DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    return TRUE;
}

// The service items run a field script, the way ItemUseOutOfBattle_TownMap does: from the
// bag, close it and start the script once the field has faded in; from Select (field
// already frozen by UseRegisteredKeyItemOnField), start it now. ScriptContext_SetupScript
// locks the player's controls; the scripts end with releaseall.

static void Task_StartRelearnerScript(u8 taskId)
{
    if (IsWeatherNotFadingIn())
    {
        ScriptContext_SetupScript(CE_EventScript_MoveRelearnerItem);
        DestroyTask(taskId);
    }
}

static void Task_StartDeleterScript(u8 taskId)
{
    if (IsWeatherNotFadingIn())
    {
        ScriptContext_SetupScript(CE_EventScript_MoveDeleterItem);
        DestroyTask(taskId);
    }
}

static void FieldCB_StartRelearnerScript(void)
{
    FadeInFromBlack();
    CreateTask(Task_StartRelearnerScript, 8);
}

static void FieldCB_StartDeleterScript(void)
{
    FadeInFromBlack();
    CreateTask(Task_StartDeleterScript, 8);
}

static void UseServiceItem(u8 taskId, void (*fieldCallback)(void), const u8 *script)
{
    if (CalculatePlayerPartyCount() == 0)
    {
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    }
    else if (!gTasks[taskId].tUsingRegisteredKeyItem)
    {
        gFieldCallback = fieldCallback;
        gBagMenu->newScreenCallback = CB2_ReturnToField;
        Task_FadeAndCloseBagMenu(taskId);
    }
    else
    {
        ScriptContext_SetupScript(script);
        DestroyTask(taskId);
    }
}

void CeItemUse_MoveRelearner(u8 taskId)
{
    UseServiceItem(taskId, FieldCB_StartRelearnerScript, CE_EventScript_MoveRelearnerItem);
}

// Not underwater: forgetting the last Dive there could leave the player unable to surface.
void CeItemUse_MoveDeleter(u8 taskId)
{
    if (gMapHeader.mapType == MAP_TYPE_UNDERWATER)
        DisplayDadsAdviceCannotUseItemMessage(taskId, gTasks[taskId].tUsingRegisteredKeyItem);
    else
        UseServiceItem(taskId, FieldCB_StartDeleterScript, CE_EventScript_MoveDeleterItem);
}

STATIC_ASSERT(CE_INFINITE_REPEL_STEPS < REPEL_LURE_MASK, CeInfiniteRepelIsARepel);

bool32 CeKeyItems_IsInfiniteRepelOn(void)
{
    return VarGet(VAR_REPEL_STEP_COUNT) == CE_INFINITE_REPEL_STEPS;
}

static void Task_CloseFieldMessage(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    DestroyTask(taskId);
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
}

void CeItemUse_InfiniteRepel(u8 taskId)
{
    const u8 *text;

    if (CeKeyItems_IsInfiniteRepelOn())
    {
        VarSet(VAR_REPEL_STEP_COUNT, 0);
        PlaySE(SE_PC_OFF);
        text = COMPOUND_STRING("The Infinite Repel was turned off.{PAUSE_UNTIL_PRESS}");
    }
    else
    {
        VarSet(VAR_REPEL_STEP_COUNT, CE_INFINITE_REPEL_STEPS);
        PlaySE(SE_REPEL);
        text = COMPOUND_STRING("The Infinite Repel was turned on.{PAUSE_UNTIL_PRESS}");
    }
    if (gTasks[taskId].tUsingRegisteredKeyItem)
        DisplayItemMessageOnField(taskId, text, Task_CloseFieldMessage);
    else
        DisplayItemMessage(taskId, FONT_NORMAL, text, CloseItemMessage);
}

#undef tUsingRegisteredKeyItem
