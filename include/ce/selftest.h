#ifndef GUARD_CE_SELFTEST_H
#define GUARD_CE_SELFTEST_H

// Boots into a probe routine when gCeRomConfig.hdr.selfTestMode is set, prints JSON
// lines through mGBA's debug registers and exits (swi 0x3, exit code in r0).
void CeSelfTest_RunIfEnabled(void);

#endif // GUARD_CE_SELFTEST_H
