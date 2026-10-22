#include "global.h"
#include "battle.h"
#include "main.h"
#include "task.h"
#include "title_screen.h"
#include "ce/qol.h"
#include "ce/speed.h"
#include "constants/battle.h"

// The battle scripts assemble their pauses from B_WAIT_TIME_MULTIPLIER, so OFF is vanilla
// timing only while it stays at 16; CE_QOL_SHORT_BATTLE_PAUSES scales them at runtime.
#if B_WAIT_TIME_MULTIPLIER != 16
#error "CE: battle pauses are scaled at runtime; keep B_WAIT_TIME_MULTIPLIER at its vanilla 16"
#endif

// Battle code only: outside a battle gBattleTypeFlags still holds the last battle's type.
static bool32 IsLinkBattle(void)
{
    return (gBattleTypeFlags & BATTLE_TYPE_LINK) != 0;
}

// Text printers run everywhere. gMain.inBattle is set before the first battle message.
static bool32 IsTextInLinkBattle(void)
{
    return gMain.inBattle && IsLinkBattle();
}

bool32 CeSpeed_IsOnInBattle(enum CeQolFeature feature)
{
    return CeQol_IsOn(feature) && !IsLinkBattle();
}

bool32 CeSpeed_NoIntroSlide(void)
{
#if TESTING
    return FALSE;
#else
    return CeSpeed_IsOnInBattle(CE_QOL_FAST_BATTLE_INTRO);
#endif
}

// GetPlayerTextSpeed runs several times per character, so keep this to the two reads.
bool32 CeSpeed_IsInstantText(void)
{
    return CeQol_IsOn(CE_QOL_INSTANT_TEXT) && !IsTextInLinkBattle();
}

bool32 CeSpeed_IsHoldToSpeedText(void)
{
    return CeQol_IsOn(CE_QOL_HOLD_TO_SPEED_TEXT) && !IsTextInLinkBattle();
}

// Halves both the B_WAIT_TIME_* pauses (B_WAIT_TIME_MULTIPLIER 8 instead of 16) and the
// few literal ones (pause 5, pause 40, waitmessage 1). 0 and 1 both end on the first frame.
u16 CeSpeed_BattleWait(u16 frames)
{
    if (!CeSpeed_IsOnInBattle(CE_QOL_SHORT_BATTLE_PAUSES))
        return frames;
    return frames / 2;
}

// SetUpCopyrightScreen has just faded to black, installed the intro's callback and task,
// and left serial on SerialCB; the save is loaded, so PLAYER modes read the player's pick.
// The title screen resets VBlank, palettes, sprites and tasks itself.
void CeSpeed_MaybeSkipBootIntro(void)
{
    if (!CeQol_IsOn(CE_QOL_SKIP_INTRO))
        return;
    ResetTasks();
    SetMainCallback2(CB2_InitTitleScreen);
}
