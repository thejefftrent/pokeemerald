#include "global.h"
#include "malloc.h"
#include "ce/qol.h"
#define CE_ROM_CONFIG_NO_TEST_OVERRIDE // copies the ROM table
#include "ce/rom_config.h"

// make check only: the RAM copy of gCeRomConfig that tests edit (see ce/rom_config.h).

#if TESTING

EWRAM_DATA struct CeRomConfig *gCeRomConfigTestOverride = NULL;

// Every QoL feature starts locked to what upstream's own config does, so expansion's test
// suite runs against upstream behaviour. CE tests switch features with CeTest_SetQol.
#define CE_QOL_UPSTREAM_MODE(id, upstream) [id] = (upstream) ? CE_QOL_MODE_ON : CE_QOL_MODE_OFF,
static const u8 sUpstreamQolModes[CE_QOL_COUNT] = { CE_QOL_FEATURES(CE_QOL_UPSTREAM_MODE) };
#undef CE_QOL_UPSTREAM_MODE

void CeTest_InitRomConfig(void)
{
    Free(gCeRomConfigTestOverride);
    gCeRomConfigTestOverride = Alloc(sizeof(gCeRomConfig));
    memcpy(gCeRomConfigTestOverride, &gCeRomConfig, sizeof(gCeRomConfig));
    memcpy(gCeRomConfigTestOverride->body.qol.mode, sUpstreamQolModes, sizeof(sUpstreamQolModes));
}

void CeTest_FreeRomConfig(void)
{
    FREE_AND_SET_NULL(gCeRomConfigTestOverride);
}

#endif // TESTING
