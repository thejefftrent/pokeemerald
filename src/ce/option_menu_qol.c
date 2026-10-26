#include "global.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "ce/key_items.h"
#include "ce/level_caps.h"
#include "ce/option_menu_qol.h"
#include "ce/qol.h"
#include "ce/rom_config.h"

// option_menu.c owns task data[0..6] and its two windows; this page adds a second
// window over the same screen area (free VRAM after the frame tiles) and keeps its
// state in task data[7..10].
#define tMenuSelection data[0]
#define tPage          data[7]
#define tCursor        data[8]
#define tScroll        data[9]
#define tWindowPlusOne data[10]

#define WIN_OPTION_HEADER  0
#define WIN_OPTION_OPTIONS 1
#define ROWS_PER_PAGE      7
#define VALUE_ON_X         104
#define VALUE_OFF_RIGHT    198
#define LABEL_X            8
#define LABEL_MAX_WIDTH    (VALUE_ON_X - LABEL_X - 10) // leave a clear gap before ON
#define ARROW_X            200

enum
{
    PAGE_VANILLA,
    PAGE_QOL,
};

static const struct WindowTemplate sQolWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 5,
    .width = 26,
    .height = 14,
    .paletteNum = 1,
    .baseBlock = 0x1B0, // after WIN_OPTIONS (0x36-0x1A1) and the frame tiles (0x1A2-0x1AA)
};

// Colours are indices into the options menu's text palette.
static const u8 sColorLabel[]      = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};   // orange
static const u8 sColorSelected[]   = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED};
static const u8 sColorUnselected[] = {TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN};    // dark grey
static const u8 sColorFaded[]      = {TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GREEN, TEXT_COLOR_WHITE};    // light grey

static const u8 *const sFeatureNames[CE_QOL_COUNT] =
{
    [CE_QOL_LEVEL_CAPS]               = COMPOUND_STRING("LEVEL CAPS"),
    [CE_QOL_RUN_INDOORS]              = COMPOUND_STRING("RUN INDOORS"),
    [CE_QOL_RUNNING_SHOES]            = COMPOUND_STRING("RUNNING SHOES"),
    [CE_QOL_AUTO_RUN]                 = COMPOUND_STRING("AUTO-RUN"),
    [CE_QOL_BIKE_INDOORS]             = COMPOUND_STRING("BIKE INDOORS"),
    [CE_QOL_FAST_SURF]                = COMPOUND_STRING("FAST SURF"),
    [CE_QOL_INSTANT_TEXT]             = COMPOUND_STRING("INSTANT TEXT"),
    [CE_QOL_HOLD_TO_SPEED_TEXT]       = COMPOUND_STRING("SPEED UP TEXT"),
    [CE_QOL_FAST_HP_BAR]              = COMPOUND_STRING("FAST HP BAR"),
    [CE_QOL_FAST_EXP_BAR]             = COMPOUND_STRING("FAST EXP BAR"),
    [CE_QOL_FAST_BATTLE_INTRO]        = COMPOUND_STRING("FAST INTROS"),
    [CE_QOL_SHORT_BATTLE_PAUSES]      = COMPOUND_STRING("SHORT PAUSES"),
    [CE_QOL_QUICK_RUN_CURSOR]         = COMPOUND_STRING("B GOES TO RUN"),
    [CE_QOL_FORFEIT_TRAINER_BATTLES]  = COMPOUND_STRING("FORFEIT BATTLES"),
    [CE_QOL_SKIP_INTRO]               = COMPOUND_STRING("SKIP INTRO"),
    [CE_QOL_QUICK_SAVE]               = COMPOUND_STRING("QUICK SAVE"),
    [CE_QOL_REUSABLE_TMS]             = COMPOUND_STRING("REUSABLE TMS"),
    [CE_QOL_ALWAYS_OBEY]              = COMPOUND_STRING("ALWAYS OBEY"),
    [CE_QOL_SUMMARY_IV_EV]            = COMPOUND_STRING("SHOW IVS/EVS"),
    [CE_QOL_SUMMARY_RELEARNER]        = COMPOUND_STRING("RELEARN MOVES"),
    [CE_QOL_EXP_SHARE_ALL]            = COMPOUND_STRING("PARTY EXP SHARE"),
    [CE_QOL_CATCH_EXP]                = COMPOUND_STRING("CATCH EXP"),
    [CE_QOL_TRADE_EVOS_WITHOUT_TRADE] = COMPOUND_STRING("NO-TRADE EVOS"),
    [CE_QOL_NATURE_MINTS]             = COMPOUND_STRING("TRAINING ITEMS"),
    [CE_QOL_NO_WILD_ENCOUNTERS]       = COMPOUND_STRING("NO WILD BATTLES"),
    [CE_QOL_NO_TRAINER_SIGHT]         = COMPOUND_STRING("NO TRAINER SIGHT"),
    [CE_QOL_HIDE_REPEAT_MAP_POPUP]    = COMPOUND_STRING("FEWER POPUPS"),
    [CE_QOL_ITEM_DESCRIPTIONS]        = COMPOUND_STRING("ITEM DETAILS"),
    [CE_QOL_CANDY_JAR]                = COMPOUND_STRING("CANDY JAR"),
    [CE_QOL_RELEARNER_ITEM]           = COMPOUND_STRING("RELEARNER ITEM"),
    [CE_QOL_DELETER_ITEM]             = COMPOUND_STRING("DELETER ITEM"),
    [CE_QOL_PC_ITEM]                  = COMPOUND_STRING("PC IN BAG"),
    [CE_QOL_FIELD_MOVES_WITHOUT_HMS]  = COMPOUND_STRING("NO HMS NEEDED"),
    [CE_QOL_FLY_FROM_ANYWHERE]        = COMPOUND_STRING("FLY ANYWHERE"),
    [CE_QOL_INFINITE_REPEL]           = COMPOUND_STRING("INFINITE REPEL"),
    [CE_QOL_FAST_EGG_HATCHING]        = COMPOUND_STRING("FAST HATCHING"),
    [CE_QOL_SHINY_CHARM]              = COMPOUND_STRING("SHINY CHARM"),
    [CE_QOL_FOLLOWERS]                = COMPOUND_STRING("FOLLOWERS"),
    [CE_QOL_MEGA_STONES]              = COMPOUND_STRING("MEGA STONES"),
    [CE_QOL_Z_CRYSTALS]               = COMPOUND_STRING("Z-CRYSTALS"),
    [CE_QOL_GEMS_PLATES_MEMORIES]     = COMPOUND_STRING("GEMS AND PLATES"),
    [CE_QOL_ROOM_SELLERS]             = COMPOUND_STRING("CENTER SELLERS"),
    [CE_QOL_BATTLE_ITEMS]             = COMPOUND_STRING("BATTLE ITEMS"),
    [CE_QOL_ALWAYS_CATCH]             = COMPOUND_STRING("ALWAYS CATCH"),
    [CE_QOL_POKEDEX_PLUS]             = COMPOUND_STRING("POKéDEX PLUS"),
    [CE_QOL_RELEARNER_EGG_TUTOR]      = COMPOUND_STRING("EGG/TUTOR RELEARN"),
    [CE_QOL_TUTOR_SELLER]             = COMPOUND_STRING("CENTER TUTOR"),
    [CE_QOL_WONDER_TRADE]             = COMPOUND_STRING("WONDER TRADE"),
};

static bool32 IsVisible(enum CeQolFeature feature)
{
    return CeQol_GetMode(feature) != CE_QOL_MODE_OFF || gCeRomConfig.body.qol.showLockedOff;
}

static u32 CountVisible(void)
{
    u32 i, count = 0;
    for (i = 0; i < CE_QOL_COUNT; i++)
    {
        if (IsVisible(i))
            count++;
    }
    return count;
}

// The index-th visible feature, in feature order.
static enum CeQolFeature VisibleAt(u32 index)
{
    u32 i;
    for (i = 0; i < CE_QOL_COUNT; i++)
    {
        if (IsVisible(i) && index-- == 0)
            return i;
    }
    return CE_QOL_COUNT;
}

static void HighlightRow(u32 row)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(row * 16 + 40, row * 16 + 56));
}

static void DrawHeader(u32 page)
{
    u8 text[32];
    u8 *end;

    FillWindowPixelBuffer(WIN_OPTION_HEADER, PIXEL_FILL(1));
    if (page == PAGE_VANILLA)
    {
        AddTextPrinterParameterized(WIN_OPTION_HEADER, FONT_NORMAL, COMPOUND_STRING("OPTION"), 8, 1, TEXT_SKIP_DRAW, NULL);
        if (CountVisible() != 0)
        {
            const u8 *hint = COMPOUND_STRING("{R_BUTTON}MORE");
            AddTextPrinterParameterized(WIN_OPTION_HEADER, FONT_NORMAL, hint, GetStringRightAlignXOffset(FONT_NORMAL, hint, 200), 1, TEXT_SKIP_DRAW, NULL);
        }
    }
    else
    {
        AddTextPrinterParameterized(WIN_OPTION_HEADER, FONT_NORMAL, COMPOUND_STRING("MORE OPTIONS"), 8, 1, TEXT_SKIP_DRAW, NULL);
        if (CeQol_IsOn(CE_QOL_LEVEL_CAPS))
        {
            u16 boss;
            u32 cap = CeLevelCap_Compute(&boss);

            if (cap >= MAX_LEVEL)
            {
                StringCopy(text, COMPOUND_STRING("NO CAP"));
            }
            else
            {
                end = StringCopy(text, COMPOUND_STRING("CAP {LV}"));
                ConvertIntToDecimalStringN(end, cap, STR_CONV_MODE_LEFT_ALIGN, 3);
            }
            AddTextPrinterParameterized(WIN_OPTION_HEADER, FONT_NORMAL, text, GetStringRightAlignXOffset(FONT_NORMAL, text, 200), 1, TEXT_SKIP_DRAW, NULL);
        }
    }
    CopyWindowToVram(WIN_OPTION_HEADER, COPYWIN_GFX);
}

static void DrawRow(u32 windowId, u32 row, enum CeQolFeature feature)
{
    const u8 *on = COMPOUND_STRING("ON");
    const u8 *off = COMPOUND_STRING("OFF");
    const u8 *name = sFeatureNames[feature] != NULL ? sFeatureNames[feature] : COMPOUND_STRING("?");
    bool32 locked = !CeQol_IsPlayerChoice(feature);
    bool32 isOn = CeQol_IsOn(feature);
    const u8 *chosen = locked ? sColorUnselected : sColorSelected;
    const u8 *other = locked ? sColorFaded : sColorUnselected;
    u32 y = row * 16 + 1;

    // Stop short of the arrow column so redrawing a row never erases a scroll arrow.
    FillWindowPixelRect(windowId, PIXEL_FILL(1), 0, row * 16, ARROW_X, 16);
    // Long labels drop to a narrower font rather than running into the ON column.
    AddTextPrinterParameterized3(windowId, GetFontIdToFit(name, FONT_NORMAL, 0, LABEL_MAX_WIDTH), LABEL_X, y,
                                 locked ? sColorUnselected : sColorLabel, TEXT_SKIP_DRAW, name);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, VALUE_ON_X, y, isOn ? chosen : other, TEXT_SKIP_DRAW, on);
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, GetStringRightAlignXOffset(FONT_NORMAL, off, VALUE_OFF_RIGHT), y,
                                 isOn ? other : chosen, TEXT_SKIP_DRAW, off);
}

static void DrawQolPage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    u32 windowId = tWindowPlusOne - 1;
    u32 count = CountVisible();
    u32 row;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    for (row = 0; row < ROWS_PER_PAGE && tScroll + row < count; row++)
        DrawRow(windowId, row, VisibleAt(tScroll + row));
    if (tScroll > 0)
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, ARROW_X, 1, sColorUnselected, TEXT_SKIP_DRAW, COMPOUND_STRING("{UP_ARROW}"));
    if (tScroll + ROWS_PER_PAGE < count)
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, ARROW_X, (ROWS_PER_PAGE - 1) * 16 + 1, sColorUnselected, TEXT_SKIP_DRAW, COMPOUND_STRING("{DOWN_ARROW}"));
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void ShowQolPage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tWindowPlusOne == 0)
        tWindowPlusOne = AddWindow(&sQolWindowTemplate) + 1;
    tPage = PAGE_QOL;
    DrawQolPage(taskId);
    PutWindowTilemap(tWindowPlusOne - 1);
    CopyWindowToVram(tWindowPlusOne - 1, COPYWIN_FULL);
    HighlightRow(tCursor - tScroll);
    DrawHeader(PAGE_QOL);
}

static void ShowVanillaPage(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    tPage = PAGE_VANILLA;
    PutWindowTilemap(WIN_OPTION_OPTIONS);
    CopyWindowToVram(WIN_OPTION_OPTIONS, COPYWIN_MAP);
    HighlightRow(tMenuSelection);
    DrawHeader(PAGE_VANILLA);
}

static void MoveCursor(u8 taskId, s32 delta)
{
    s16 *data = gTasks[taskId].data;
    s32 count = CountVisible();
    s32 cursor = tCursor + delta;
    s32 scroll = tScroll;

    if (cursor < 0)
        cursor = count - 1;
    else if (cursor >= count)
        cursor = 0;
    if (cursor < scroll)
        scroll = cursor;
    else if (cursor >= scroll + ROWS_PER_PAGE)
        scroll = cursor - ROWS_PER_PAGE + 1;

    tCursor = cursor;
    if (scroll != tScroll)
    {
        tScroll = scroll;
        DrawQolPage(taskId);
    }
    HighlightRow(tCursor - tScroll);
}

void CeOptions_OnMenuInit(u8 taskId, s16 *textSpeed, s16 *battleSceneOff, s16 *battleStyle)
{
    // The vanilla page only offers SLOW/MID/FAST; a lock to INSTANT shows as FAST.
    *textSpeed = min(CeOptions_Effective(CE_VOPT_TEXT_SPEED, *textSpeed), OPTIONS_TEXT_SPEED_FAST);
    *battleSceneOff = CeOptions_Effective(CE_VOPT_BATTLE_SCENE, *battleSceneOff);
    *battleStyle = CeOptions_Effective(CE_VOPT_BATTLE_STYLE, *battleStyle);
    DrawHeader(PAGE_VANILLA);
}

bool32 CeOptions_ProcessInput(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tPage == PAGE_VANILLA)
    {
        if (JOY_NEW(R_BUTTON) && CountVisible() != 0)
        {
            ShowQolPage(taskId);
            return TRUE;
        }
        // Locked rows can be passed over and exited from, but never changed.
        if (tMenuSelection < CE_VOPT_COUNT && CeOptions_IsLocked(tMenuSelection)
         && !JOY_NEW(A_BUTTON | B_BUTTON | DPAD_UP | DPAD_DOWN))
            return TRUE;
        return FALSE;
    }

    if (JOY_NEW(B_BUTTON))
        return FALSE; // vanilla: save and exit
    if (JOY_NEW(R_BUTTON))
        ShowVanillaPage(taskId);
    else if (JOY_NEW(DPAD_UP))
        MoveCursor(taskId, -1);
    else if (JOY_NEW(DPAD_DOWN))
        MoveCursor(taskId, 1);
    else if (JOY_NEW(A_BUTTON | DPAD_LEFT | DPAD_RIGHT))
    {
        enum CeQolFeature feature = VisibleAt(tCursor);
        if (feature < CE_QOL_COUNT && CeQol_IsPlayerChoice(feature))
        {
            CeQol_SetPlayerValue(feature, !CeQol_IsOn(feature));
            DrawRow(tWindowPlusOne - 1, tCursor - tScroll, feature);
            CopyWindowToVram(tWindowPlusOne - 1, COPYWIN_GFX);
            if (feature == CE_QOL_LEVEL_CAPS)
                DrawHeader(PAGE_QOL);
        }
    }
    return TRUE;
}

void CeOptions_OnMenuSave(u8 taskId, s16 *textSpeed, s16 *battleSceneOff, s16 *battleStyle)
{
    // Locked rows write back what the save already had, so the player's own preference
    // returns if a later patch unlocks them.
    if (CeOptions_IsLocked(CE_VOPT_TEXT_SPEED))
        *textSpeed = gSaveBlock2Ptr->optionsTextSpeed;
    if (CeOptions_IsLocked(CE_VOPT_BATTLE_SCENE))
        *battleSceneOff = gSaveBlock2Ptr->optionsBattleSceneOff;
    if (CeOptions_IsLocked(CE_VOPT_BATTLE_STYLE))
        *battleStyle = gSaveBlock2Ptr->optionsBattleStyle;

    // Toggled key-item features take effect now when in-game; from the title screen the
    // next Continue or New Game syncs them.
    if (gMain.savedCallback == CB2_ReturnToFieldWithOpenMenu)
        CeKeyItems_Sync();
}

void CeOptions_GreyLockedChoice(u8 *dst, u32 row, u32 selected)
{
    if (row >= CE_VOPT_COUNT || !CeOptions_IsLocked(row))
        return;
    // dst starts with {COLOR x}{SHADOW y}: bytes 2 and 5 are the colours.
    dst[2] = selected ? TEXT_COLOR_GREEN : TEXT_COLOR_LIGHT_GREEN;
    dst[5] = selected ? TEXT_COLOR_LIGHT_GREEN : TEXT_COLOR_WHITE;
}

void CeOptions_GreyLockedLabels(u32 windowId, const u8 *const *names)
{
    u32 row;
    for (row = 0; row < CE_VOPT_COUNT; row++)
    {
        if (!CeOptions_IsLocked(row))
            continue;
        FillWindowPixelRect(windowId, PIXEL_FILL(1), 0, row * 16, VALUE_ON_X, 16);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, 8, row * 16 + 1, sColorUnselected, TEXT_SKIP_DRAW, names[row]);
    }
}
