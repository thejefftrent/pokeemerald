#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "save.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "ce/modes.h"
#include "constants/rgb.h"

// Nuzlocke with whiteoutEndsRun: whiting out ends the run. CB2_WhiteOut hands over to this
// screen instead of warping to the last Pokémon Center. It says the run is lost, erases the
// save (every flash sector, like the title screen's "clear save data") and resets to the title
// screen, where only New Game is left. The save is erased before the message asks for a
// button, so switching off at the message doesn't keep it.
//
// Every whiteout passes through CB2_WhiteOut: lost wild, trainer and scripted battles, the
// Steven multi battle and field poison. The battles the rules exempt never get there (the
// Frontier, Trainer Hill, Battle Tents and secret bases return to their scripts, Birch's
// rescue goes on, link battles end elsewhere).

enum
{
    STATE_SETUP,
    STATE_FADE_IN,
    STATE_ERASE,
    STATE_MESSAGE,
    STATE_FADE_OUT,
};

#define WIN_TEXT      0
#define FRAME_TILE    2
#define FRAME_PALETTE 14

// Two lines of 26 tiles; the first stays while the second changes.
static const u8 sText_Erasing[] = _("The NUZLOCKE challenge is lost.\nErasing the save file…");
static const u8 sText_Erased[] = _("The NUZLOCKE challenge is lost.\nThe save file has been erased.\p");

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sWindowTemplates[] =
{
    [WIN_TEXT] = {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = FRAME_TILE + 9, // after the frame's tiles
    },
    DUMMY_WIN_TEMPLATE
};

bool32 CeModes_WhiteoutEndsRun(void)
{
    const struct CeModesConfig *rules = CeModes_Rules();

    return rules->nuzlocke && rules->whiteoutEndsRun;
}

bool32 CeModes_TryEndRunOnWhiteout(void)
{
    if (!CeModes_WhiteoutEndsRun())
        return FALSE;
    SetMainCallback1(NULL); // the overworld's, after field poison
    SetMainCallback2(CB2_CeRunLost);
    return TRUE;
}

static void VBlankCB_RunLost(void)
{
    TransferPlttBuffer();
}

static void PrintMessage(const u8 *text, u8 speed)
{
    FillWindowPixelBuffer(WIN_TEXT, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TEXT, FONT_NORMAL, text, 0, 1, speed, NULL);
    CopyWindowToVram(WIN_TEXT, COPYWIN_FULL);
}

static void SetUpScreen(void)
{
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)PLTT, PLTT_SIZE); // black backdrop
    StopMapMusic();
    ResetPaletteFade();
    ResetTasks();
    ResetSpriteData();
    FreeAllWindowBuffers();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    InitWindows(sWindowTemplates);
    DeactivateAllTextPrinters();
    LoadWindowGfx(WIN_TEXT, gSaveBlock2Ptr->optionsWindowFrameType, FRAME_TILE, BG_PLTT_ID(FRAME_PALETTE));
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    DrawStdFrameWithCustomTileAndPalette(WIN_TEXT, FALSE, FRAME_TILE, FRAME_PALETTE);
    PutWindowTilemap(WIN_TEXT);
    PrintMessage(sText_Erasing, 0);
    CopyBgTilemapBufferToVram(0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    ShowBg(0);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_RunLost);
}

void CB2_CeRunLost(void)
{
    switch (gMain.state)
    {
    case STATE_SETUP:
        SetUpScreen();
        gMain.state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
            gMain.state = STATE_ERASE;
        break;
    case STATE_ERASE:
        ClearSaveData();
        PrintMessage(sText_Erased, GetPlayerTextSpeedDelay());
        gMain.state = STATE_MESSAGE;
        break;
    case STATE_MESSAGE:
        // The message ends with a page break, so the printer waits for A or B.
        RunTextPrinters();
        if (!IsTextPrinterActiveOnWindow(WIN_TEXT))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            gMain.state = STATE_FADE_OUT;
        }
        break;
    case STATE_FADE_OUT:
        UpdatePaletteFade();
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            DoSoftReset();
        }
        break;
    }
}
