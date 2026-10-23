#include "global.h"
#include "event_data.h"
#include "script.h"
#include "ce/logic.h"
#include "ce/rom_config.h"
#include "constants/flags.h"

// Mr. Briney's ferry. Vanilla stops it for good when Norman is beaten (the HM03 Surf gift comes
// right after); the key-item logic can't count on Surf by then, so it keeps the ferry running,
// as it does before Norman: his boat stays where it is, the Pokémon Centers keep track of
// where that is, and a Dewford heal location stays (he can still take you off the island).

extern const u8 EventScript_HideMrBriney[];
extern const u8 Common_EventScript_NopReturn[];
void ResetHealLocationFromDewford(void);

bool32 CeLogic_KeepsBriney(void)
{
    return gCeRomConfig.body.logic.keepBriney != 0;
}

void CeLogic_ScriptNormanResetHealLocation(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
    if (!CeLogic_KeepsBriney())
        ResetHealLocationFromDewford();
}

void CeLogic_ScriptNormanHideMrBriney(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (!CeLogic_KeepsBriney())
        ScriptCall(ctx, EventScript_HideMrBriney);
}

void CeLogic_ScriptBrineyGoneAfterNorman(struct ScriptContext *ctx)
{
    Script_RequestEffects(SCREFF_V1);
    if (FlagGet(FLAG_DEFEATED_PETALBURG_GYM) && !CeLogic_KeepsBriney())
        ScriptJump(ctx, Common_EventScript_NopReturn);
}
