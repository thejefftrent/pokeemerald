#ifndef GUARD_CE_SELFTEST_H
#define GUARD_CE_SELFTEST_H

// Boots into a probe routine when gCeRomConfig.hdr.selfTestMode is set, prints JSON
// lines through mGBA's debug registers and exits (swi 0x3, exit code in r0).
void CeSelfTest_RunIfEnabled(void);

// Building a probe line: CeSelfTest_LineStart("name") then CeSelfTest_PutField(...) for each
// value, then CeSelfTest_LineEmit(). Prints CE:{"probe":"name",...} to mGBA's debug log.
void CeSelfTest_LineStart(const char *probe);
void CeSelfTest_PutField(const char *name, u32 value);
void CeSelfTest_LineEmit(void);

// Per-area probes, each defined in its area's own src/ce/selftest_<area>.c. They are weak,
// so an area without probes simply doesn't define one. Called in this order after the
// species, starters, level-cap and QoL probes.
void CeSelfTest_ProbeLearnsets(void) __attribute__((weak));
void CeSelfTest_ProbeTrainers(void) __attribute__((weak));
void CeSelfTest_ProbeWild(void) __attribute__((weak));
void CeSelfTest_ProbeStatics(void) __attribute__((weak));
void CeSelfTest_ProbeModes(void) __attribute__((weak));
void CeSelfTest_ProbeBattleConfig(void) __attribute__((weak));
void CeSelfTest_ProbeItems(void) __attribute__((weak));
void CeSelfTest_ProbeStart(void) __attribute__((weak));
void CeSelfTest_ProbeSeedHash(void) __attribute__((weak));
void CeSelfTest_ProbeTeam(void) __attribute__((weak));
void CeSelfTest_ProbeLogic(void) __attribute__((weak));
void CeSelfTest_ProbeThirdType(void) __attribute__((weak));
void CeSelfTest_ProbeSprites(void) __attribute__((weak));

#endif // GUARD_CE_SELFTEST_H
