#ifndef GUARD_CE_ROM_CONFIG_H
#define GUARD_CE_ROM_CONFIG_H

// Custom Emerald patch interface.
// The website writes this block after the build; game code only ever reads it.
// Field offsets reach the website through DWARF, so fields can be added freely,
// but the block keeps a fixed size so adding fields never moves other data.
// Zero must always mean "vanilla behaviour".

#define CE_ROM_CONFIG_SIZE      0x800
#define CE_ROM_CONFIG_LAYOUT    1
#define CE_SEED_HASH_LENGTH     5
#define CE_SETTINGS_CODE_LENGTH 32

struct CeRomConfigHeader
{
    u8 magic[8];         // "CECFG"
    u16 layoutVersion;   // CE_ROM_CONFIG_LAYOUT
    u16 size;            // sizeof(struct CeRomConfig)
    u32 configHash;      // hash of the settings that produced this ROM; 0 in the base ROM
    u32 seed;
    u16 seedHash[CE_SEED_HASH_LENGTH]; // species shown in-game as the seed's hash
    u8 selfTestMode;     // nonzero: boot into the self-test instead of the game
    u8 padding;
    u8 settingsCode[CE_SETTINGS_CODE_LENGTH];
};

#define CE_SELF_TEST_SPECIES 8

// Only read when hdr.selfTestMode != 0 (CI builds a test ROM to prove patches take effect).
struct CeSelfTestConfig
{
    u16 species[CE_SELF_TEST_SPECIES]; // SPECIES_NONE ends the list
};

struct CeRomConfigBody
{
    struct CeSelfTestConfig selfTest;
};

struct CeRomConfig
{
    struct CeRomConfigHeader hdr;
    struct CeRomConfigBody body;
    u8 reserved[CE_ROM_CONFIG_SIZE - sizeof(struct CeRomConfigHeader) - sizeof(struct CeRomConfigBody)];
};

extern const struct CeRomConfig gCeRomConfig;

#endif // GUARD_CE_ROM_CONFIG_H
