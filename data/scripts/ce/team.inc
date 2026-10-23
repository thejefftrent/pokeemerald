#include "constants/ce_team.h"
@ Static team and badge-timed PC deliveries (src/ce/team.c).

@ Called right after a PC boots up (every PC, the Pokémon Box Link and the bedroom PCs):
@ hands out each Pokémon and PC item whose badges are met and says what arrived. VAR_0x8006
@ counts the ones that found no room, so each of those is mentioned once and skipped after.
CeEventScript_PcDeliveries::
	setvar VAR_0x8006, 0
CeEventScript_PcDeliveriesNext:
	callnative CeTeam_ScriptDeliverNext, requests_effects=1
	goto_if_eq VAR_RESULT, CE_DELIVERY_NONE, CeEventScript_PcDeliveriesDone
	call_if_eq VAR_RESULT, CE_DELIVERY_MON, CeEventScript_PcMonArrived
	call_if_eq VAR_RESULT, CE_DELIVERY_MON_MOVED, CeEventScript_PcMonMoved
	call_if_eq VAR_RESULT, CE_DELIVERY_MON_NO_ROOM, CeEventScript_PcMonNoRoom
	call_if_eq VAR_RESULT, CE_DELIVERY_ITEM, CeEventScript_PcItemArrived
	call_if_eq VAR_RESULT, CE_DELIVERY_ITEM_NO_ROOM, CeEventScript_PcItemNoRoom
	goto CeEventScript_PcDeliveriesNext
CeEventScript_PcDeliveriesDone:
	return

CeEventScript_PcMonArrived:
	playfanfare MUS_OBTAIN_ITEM
	message CeText_PcMonArrived
	waitmessage
	waitfanfare
	waitbuttonpress
	return

CeEventScript_PcMonMoved:
	playfanfare MUS_OBTAIN_ITEM
	message CeText_PcMonMoved
	waitmessage
	waitfanfare
	waitbuttonpress
	return

CeEventScript_PcMonNoRoom:
	msgbox CeText_PcMonNoRoom, MSGBOX_DEFAULT
	addvar VAR_0x8006, 1
	return

CeEventScript_PcItemArrived:
	msgbox CeText_PcItemArrived, MSGBOX_DEFAULT
	return

CeEventScript_PcItemNoRoom:
	msgbox CeText_PcItemNoRoom, MSGBOX_DEFAULT
	addvar VAR_0x8006, 1
	return

@ Gift Pokémon and eggs (CeTeam_ScriptGiftGate), and the "no more room" every givemon
@ script falls back to (CeTeam_ScriptNoRoomGate): say why instead.
CeEventScript_GiftRefused::
CeEventScript_NoRoomStaticTeam::
	msgbox CeText_NoNewPokemon, MSGBOX_DEFAULT
	release
	end

@ ingame_trade (CeTeam_ScriptTradeGate).
CeEventScript_TradeRefused::
	msgbox CeText_TradeRefused, MSGBOX_DEFAULT
	release
	end

@ Route 121's Safari Zone counter, after the player says yes (CeTeam_ScriptSafariGate).
CeEventScript_SafariRefused::
	msgbox CeText_SafariRefused, MSGBOX_DEFAULT
	goto Route121_SafariZoneEntrance_EventScript_MovePlayerBackFromCounter
	end

@ Birch's lab after the rescue (CeTeam_ScriptStarterGate): the starter wasn't given, so
@ there is nothing to hand over or nickname.
CeEventScript_StarterKept::
	msgbox CeText_StarterKept, MSGBOX_DEFAULT
	goto LittlerootTown_ProfessorBirchsLab_EventScript_GoSeeRival
	end

CeText_PcMonArrived:
	.string "{STR_VAR_1} joined your team!\n"
	.string "It's waiting for you in {STR_VAR_2}.$"

CeText_PcMonMoved:
	.string "{STR_VAR_1} joined your team!\p"
	.string "Its spot was taken, so it's\n"
	.string "waiting for you in {STR_VAR_2}.$"

CeText_PcMonNoRoom:
	.string "{STR_VAR_1} is ready to join your\n"
	.string "team, but every BOX is full!\p"
	.string "Make room in the PC for it.$"

CeText_PcItemArrived:
	.string "Your item storage received\n"
	.string "{STR_VAR_3} {STR_VAR_1}!$"

CeText_PcItemNoRoom:
	.string "{STR_VAR_1} is waiting for you,\n"
	.string "but the item storage is full!$"

CeText_NoNewPokemon:
	.string "Your static team can't take in\n"
	.string "new POKéMON on this run!$"

CeText_TradeRefused:
	.string "Your static team can't trade for\n"
	.string "new POKéMON on this run!$"

CeText_SafariRefused:
	.string "The SAFARI ZONE is for catching\n"
	.string "POKéMON.\p"
	.string "Your static team can't take in\n"
	.string "new POKéMON on this run!$"

CeText_StarterKept:
	.string "PROF. BIRCH: Your own team did\n"
	.string "great out there, {PLAYER}!\p"
	.string "I'll keep the POKéMON in my bag\n"
	.string "here at the LAB, then.$"
