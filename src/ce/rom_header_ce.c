#include "global.h"
#define CE_ROM_CONFIG_NO_TEST_OVERRIDE // points at the ROM table
#include "ce/rom_config.h"
#include "ce/build_info.h"

// Like the GF and RHH headers: lets tools (and the website) recognise a Custom
// Emerald base or generated ROM by scanning the first KiB for the magic.
// Append-only: never reorder or resize existing fields.

#define CE_ROM_HEADER_VERSION 1

extern const u8 __ce_free_start[];
extern const u8 __ce_free_end[];

struct CeRomHeader
{
    /*0x00*/ char magic[8]; // 'CUSTEMER'
    /*0x08*/ u16 headerVersion;
    /*0x0A*/ u16 configLayoutVersion;
    /*0x0C*/ u32 forkCommit;
    /*0x10*/ char baseId[24];
    /*0x28*/ const struct CeRomConfig *config;
    /*0x2C*/ const u8 *freeStart;
    /*0x30*/ const u8 *freeEnd;
};

__attribute__((section(".text.header_ce"))) USED static const struct CeRomHeader sCeRomHeader =
{
    .magic = { 'C', 'U', 'S', 'T', 'E', 'M', 'E', 'R' },
    .headerVersion = CE_ROM_HEADER_VERSION,
    .configLayoutVersion = CE_ROM_CONFIG_LAYOUT,
    .forkCommit = CE_FORK_COMMIT,
    .baseId = CE_BASE_ID,
    .config = &gCeRomConfig,
    .freeStart = __ce_free_start,
    .freeEnd = __ce_free_end,
};
