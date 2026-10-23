#ifndef GUARD_CE_TEST_H
#define GUARD_CE_TEST_H

// Helpers for Custom Emerald's tests in test/ce (make check; docs/base-tests.md).
//
// Every test starts from a cleared save and from a RAM copy of the ROM's gCeRomConfig in which
// each QoL feature is locked to what upstream's own config does (ce/rom_config.h). The game
// reads that copy wherever it reads gCeRomConfig, so a test sets only what it is about. Battle
// tests set it inside GIVEN; function tests set it in every PARAMETRIZE run (the copy is made
// once per test, not once per run).
//
// Name CE tests "CE: ..." so that TESTS='CE:' runs only ours.

#include "ce/modes.h"
#include "ce/qol.h"
#include "ce/rom_config.h"

// The config this test's game reads (struct CeRomConfig).
#define CE_TEST_CONFIG (gCeRomConfigTestOverride)

static inline void CeTest_SetQol(enum CeQolFeature feature, enum CeQolMode mode)
{
    CE_TEST_CONFIG->body.qol.mode[feature] = mode;
}

// Patch these rules into the ROM and start a run, as New Game does: the save takes a copy.
static inline void CeTest_StartRun(const struct CeModesConfig *rules)
{
    CE_TEST_CONFIG->body.modes = *rules;
    CeModes_OnNewGame();
}

#endif // GUARD_CE_TEST_H
