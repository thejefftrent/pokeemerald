#include "global.h"
#include "ce/key_items.h"
#include "ce/qol.h"
#include "ce/rom_config.h"

// Resolved at read time: locked modes come straight from ROM, so a save can never
// override a competitive ROM's rules, and only PLAYER modes look at the save.

STATIC_ASSERT(CE_QOL_COUNT <= CE_QOL_MAX, CeQolFitsConfig);
STATIC_ASSERT(CE_QOL_MAX <= 32 * ARRAY_COUNT(((struct Pokedex *)0)->ceQolChosen), CeQolFitsSave);
STATIC_ASSERT(offsetof(struct SaveBlock2, pokedex.ceQolChosen) == 0x28, CeQolChosenOffset);
STATIC_ASSERT(offsetof(struct SaveBlock2, pokedex.ceQolValue) == 0x30, CeQolValueOffset);
STATIC_ASSERT(offsetof(struct SaveBlock2, filler_90) == 0x90, CeFiller90Unmoved);

#if TESTING
#define CE_QOL_UPSTREAM_ENTRY(id, upstream) [id] = upstream,
static const bool8 sCeQolUpstream[CE_QOL_COUNT] = { CE_QOL_FEATURES(CE_QOL_UPSTREAM_ENTRY) };
#undef CE_QOL_UPSTREAM_ENTRY
#endif

enum CeQolMode CeQol_GetMode(enum CeQolFeature feature)
{
    if (feature >= CE_QOL_COUNT)
        return CE_QOL_MODE_OFF;
    return gCeRomConfig.body.qol.mode[feature];
}

bool32 CeQol_IsPlayerChoice(enum CeQolFeature feature)
{
    enum CeQolMode mode = CeQol_GetMode(feature);
    return mode == CE_QOL_MODE_PLAYER_OFF || mode == CE_QOL_MODE_PLAYER_ON;
}

bool32 CeQol_IsOn(enum CeQolFeature feature)
{
#if TESTING
    return feature < CE_QOL_COUNT && sCeQolUpstream[feature];
#else
    u32 word = feature / 32, bit = 1u << (feature % 32);

    switch (CeQol_GetMode(feature))
    {
    case CE_QOL_MODE_ON:
        return TRUE;
    case CE_QOL_MODE_PLAYER_OFF:
    case CE_QOL_MODE_PLAYER_ON:
        if (gSaveBlock2Ptr->pokedex.ceQolChosen[word] & bit)
            return (gSaveBlock2Ptr->pokedex.ceQolValue[word] & bit) != 0;
        return CeQol_GetMode(feature) == CE_QOL_MODE_PLAYER_ON;
    default:
        return FALSE;
    }
#endif
}

void CeQol_SetPlayerValue(enum CeQolFeature feature, bool32 on)
{
    u32 word = feature / 32, bit = 1u << (feature % 32);

    if (!CeQol_IsPlayerChoice(feature))
        return;
    gSaveBlock2Ptr->pokedex.ceQolChosen[word] |= bit;
    if (on)
        gSaveBlock2Ptr->pokedex.ceQolValue[word] |= bit;
    else
        gSaveBlock2Ptr->pokedex.ceQolValue[word] &= ~bit;
}

bool32 CeOptions_IsLocked(enum CeVanillaOption option)
{
#if TESTING
    return FALSE;
#else
    if (option == CE_VOPT_BATTLE_STYLE && CeModes_ForcesSetStyle()) return TRUE; // CE modes: hard mode, from the save
    return option < CE_VOPT_COUNT && gCeRomConfig.body.qol.vanillaOptionLock[option] != 0;
#endif
}

u32 CeOptions_Effective(enum CeVanillaOption option, u32 saved)
{
    if (option == CE_VOPT_BATTLE_STYLE && CeModes_ForcesSetStyle()) return OPTIONS_BATTLE_STYLE_SET; // CE modes: hard mode, from the save
    if (!CeOptions_IsLocked(option))
        return saved;
    return gCeRomConfig.body.qol.vanillaOptionLock[option] - 1;
}

void CeQol_OnNewGame(void)
{
    CeKeyItems_Sync();
}

void CeQol_OnContinue(void)
{
    CeKeyItems_Sync();
}
