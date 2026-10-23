#include "global.h"
#define CE_ROM_CONFIG_NO_TEST_OVERRIDE // defines the ROM table
#include "ce/rom_config.h"

// Data only: no code in this file may read gCeRomConfig, so the compiler never
// sees an initializer it could fold. Everything else sees it through `extern`.

STATIC_ASSERT(sizeof(struct CeRomConfig) == CE_ROM_CONFIG_SIZE, CeRomConfigSize)

__attribute__((section(".rodata.ce_config"), aligned(4))) CE_PATCHABLE const struct CeRomConfig gCeRomConfig =
{
    .hdr =
    {
        .magic = { 'C', 'E', 'C', 'F', 'G' },
        .layoutVersion = CE_ROM_CONFIG_LAYOUT,
        .size = sizeof(struct CeRomConfig),
    },
};
