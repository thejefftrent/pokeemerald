@ Game modes (src/ce/modes.c): where the no-heal rules send the Pokémon Center nurse
@ (CeModes_ScriptCenterHealGate) and the out-of-center heals of Mom, beds and rest stops
@ (CeModes_ScriptRestHealGate) instead of healing, and where the no-experience rules send
@ the Day Care lady (CeModes_ScriptDaycareGate) instead of taking a Pokémon.

CeEventScript_CenterHealRefused::
	msgbox CeText_CenterHealRefused, MSGBOX_DEFAULT
	release
	end

@ Jumped to from inside Common_EventScript_OutOfCenterPartyHeal, so it returns to its caller.
CeEventScript_RestHealRefused::
	msgbox CeText_RestHealRefused, MSGBOX_DEFAULT
	closemessage
	return

CeEventScript_DaycareRefused::
	msgbox CeText_DaycareRefused, MSGBOX_DEFAULT
	release
	end

CeText_CenterHealRefused:
	.string "I'm sorry, but the rules of your\n"
	.string "challenge don't allow me to heal\l"
	.string "your POKéMON.\p"
	.string "We hope to see you again!$"

CeText_RestHealRefused:
	.string "The rules of your challenge don't\n"
	.string "allow resting to heal your POKéMON.$"

CeText_DaycareRefused:
	.string "I'm sorry, but the rules of your\n"
	.string "challenge don't allow us to\l"
	.string "raise your POKéMON.\p"
	.string "Come again.$"
