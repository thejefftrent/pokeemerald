#ifndef GUARD_CE_ROM_CONFIG_H
#define GUARD_CE_ROM_CONFIG_H

#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/start.h"
#include "ce/logic.h"
#include "ce/wild.h"

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

struct CeQolConfig
{
    u8 mode[CE_QOL_MAX];                 // enum CeQolMode, indexed by enum CeQolFeature
    u8 showLockedOff;                    // also list ALWAYS_OFF features on the in-game QoL page
    u8 vanillaOptionLock[CE_VOPT_COUNT]; // 0 = player's choice, n = forced to value n - 1
};

#define CE_LEVEL_CAP_MAX_BOSSES 16

// Level caps (CE_QOL_LEVEL_CAPS): the cap is the highest level in the party of the first
// boss in this list the player has not beaten (and of every boss before it), plus levelOffset.
struct CeLevelCapConfig
{
    s8 levelOffset;
    u8 bossCount;                         // 0 = the built-in list (8 gym leaders, Elite Four, Wallace)
    u16 bosses[CE_LEVEL_CAP_MAX_BOSSES];  // trainer ids in progression order
};

#define CE_TRAINER_PROBES 4
#define CE_PRICE_PROBES   2

// Trainers area. Trainer data itself is edited in gTrainers (CE_PATCHABLE); this only holds
// what CeSelfTest_ProbeTrainers checks, and is only read when hdr.selfTestMode != 0.
struct CeTrainerConfig
{
    u16 probeTrainers[CE_TRAINER_PROBES]; // trainers whose parties to build; TRAINER_NONE ends the list
    u16 probeItems[CE_PRICE_PROBES];      // items whose price to report; ITEM_NONE ends the list
};

struct CeRomConfigBody
{
    struct CeSelfTestConfig selfTest;
    struct CeQolConfig qol;
    struct CeLevelCapConfig levelCaps;
    struct CeTrainerConfig trainers;
    struct CeWildConfig wild;
    struct CeModesConfig modes;
    struct CeStartConfig start;
    struct CeLogicConfig logic;
};

struct CeRomConfig
{
    struct CeRomConfigHeader hdr;
    struct CeRomConfigBody body;
    u8 reserved[CE_ROM_CONFIG_SIZE - sizeof(struct CeRomConfigHeader) - sizeof(struct CeRomConfigBody)];
};

extern const struct CeRomConfig gCeRomConfig;

#endif // GUARD_CE_ROM_CONFIG_H
