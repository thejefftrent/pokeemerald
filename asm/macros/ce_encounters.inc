	@ Custom Emerald: gift, static and starter encounters come from gCeEncounters
	@ (ce/encounters.json, src/ce/encounters.c), which the website rewrites. Ids are CE_ENC_*
	@ from include/constants/ce_encounters.h. tools/ce/check_encounter_scripts.sh fails the
	@ build check if a map script still gives or battles a literal SPECIES_ constant.

	@ Loads encounter `id` into VAR_CE_ENC_SPECIES, VAR_CE_ENC_LEVEL and VAR_CE_ENC_ITEM, for
	@ commands that accept vars: givemon, giveegg, playmoncry, bufferspeciesname, showmonpic, copyvar.
	@ VAR_CE_ENC_SPECIES is otherwise unused; the level and item vars are scratch (VAR_0x800A/B),
	@ so use them right away.
	.macro ce_loadenc id:req
	callnative CeScript_LoadEncounter, requests_effects=1
	.2byte \id
	.endm

	@ givemon for encounter `id`: species, level, held item and flags. Sets VAR_RESULT like givemon.
	.macro ce_givemon id:req
	ce_loadenc \id
	givemon VAR_CE_ENC_SPECIES, VAR_CE_ENC_LEVEL, VAR_CE_ENC_ITEM
	callnative CeScript_ApplyGiftFlags, requests_effects=1
	.2byte \id
	.endm

	@ giveegg for encounter `id`.
	.macro ce_giveegg id:req
	ce_loadenc \id
	giveegg VAR_CE_ENC_SPECIES
	.endm

	@ setwildbattle for encounter `id` (setwildbattle itself only takes constants). Single battles
	@ only, like every static encounter in Emerald; follow with dowildbattle or a battle special.
	.macro setwildbattle_ce id:req
	callnative CeScript_SetWildBattle, requests_effects=1
	.2byte \id
	.endm

	@ seteventmon for encounter `id`.
	.macro seteventmon_ce id:req
	callnative CeScript_SetEventMon, requests_effects=1
	.2byte \id
	.endm
