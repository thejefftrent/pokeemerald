#include "global.h"
#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/rom_config.h"
#include "ce/rules_warning.h"

STATIC_ASSERT(CE_QOL_COUNT < CE_QOL_MAX, CeRulesWarnedBitIsNoFeature);

#define WARNED_WORD (CE_RULES_WARNED_BIT / 32)
#define WARNED_MASK (1u << (CE_RULES_WARNED_BIT % 32))

// Main menu error window: 26 tiles wide, two lines per page.
static const u8 sText_RulesChanged[] = _("This ROM's challenge rules differ\nfrom the ones your save started with.\pYour save keeps its original rules.");

bool32 CeRulesWarning_RulesDiffer(void)
{
    return memcmp(CeModes_Rules(), &gCeRomConfig.body.modes, sizeof(struct CeModesConfig)) != 0;
}

bool32 CeRulesWarning_ShouldShow(void)
{
    u32 *warned = &gSaveBlock2Ptr->pokedex.ceQolValue[WARNED_WORD];

    if (!CeRulesWarning_RulesDiffer())
    {
        *warned &= ~WARNED_MASK;
        return FALSE;
    }
    if (*warned & WARNED_MASK)
        return FALSE;
    *warned |= WARNED_MASK;
    return TRUE;
}

const u8 *CeRulesWarning_Text(void)
{
    return sText_RulesChanged;
}
