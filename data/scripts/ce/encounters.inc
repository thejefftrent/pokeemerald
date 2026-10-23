@ Self-test only (CeSelfTest_ProbeStatics in src/ce/selftest_statics.c runs these with
@ RunScriptImmediately): the encounter macros the map scripts use, on fixed encounters.

CeSelfTest_EventScript_StaticsGift::
	ce_givemon CE_ENC_BELDUM
	end

CeSelfTest_EventScript_StaticsEgg::
	ce_giveegg CE_ENC_WYNAUT_EGG
	end

CeSelfTest_EventScript_StaticsWild::
	setwildbattle_ce CE_ENC_REGICE
	end

CeSelfTest_EventScript_StaticsEvent::
	seteventmon_ce CE_ENC_LATIAS_SOUTHERN_ISLAND
	end
