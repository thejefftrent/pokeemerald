#ifndef GUARD_CE_LOGIC_H
#define GUARD_CE_LOGIC_H

// Key-item and HM logic randomizer (settings.logic). The website moves key items and HMs with
// the items area's data and writes gCeRomConfig.body.logic for the few things the logic needs
// the game to do differently. This header is included from ce/rom_config.h, so it only uses
// basic types.
//
// Every field is 0 for vanilla.

struct CeLogicConfig
{
    u8 keepBriney; // 1: Mr. Briney keeps sailing after Norman (vanilla stops his ferry for good)
    u8 padding[3];
};

struct ScriptContext;

// Whether Mr. Briney keeps sailing after Norman.
bool32 CeLogic_KeepsBriney(void);

// Script hooks (callnative). Norman's win: reset a Dewford heal location and hide Mr. Briney,
// unless he keeps sailing (PetalburgCity_Gym). The Pokémon Center check of where he is
// (Common_EventScript_UpdateBrineyLocation): stop after Norman, unless he keeps sailing.
void CeLogic_ScriptNormanResetHealLocation(struct ScriptContext *ctx);
void CeLogic_ScriptNormanHideMrBriney(struct ScriptContext *ctx);
void CeLogic_ScriptBrineyGoneAfterNorman(struct ScriptContext *ctx);

#endif // GUARD_CE_LOGIC_H
