#include "constants/ce_wonder_trade.h"
@ CE_QOL_WONDER_TRADE: the Wonder Trade attendant upstairs in every Pokemon Center (the FLAG_TEMP_C
@ seller of sPcSellers, src/ce/progression.c; its object is at the end of each 2F map.json, and
@ sPcSellers hides it in Nuzlocke, single-Pokemon and static team runs). See include/ce/wonder_trade.h.
@ Free and unlimited: the player sends a Pokemon from the party or the PC and gets a random one at
@ the same level, through the in-game trade scene. Included by data/event_scripts.s with cpp: no
@ apostrophes in these comments.

CE_EventScript_PcSellerWonderTrade::
	lock
	faceplayer
	callnative CeWonderTrade_ScriptGate, requests_effects=1
	switch VAR_RESULT
	case CE_WT_REFUSED_OFF, CE_EventScript_WonderTrade_Off
	case CE_WT_REFUSED_RULES, CE_EventScript_WonderTrade_Rules
	case CE_WT_REFUSED_STATIC_TEAM, CE_EventScript_WonderTrade_StaticTeam
	case CE_WT_REFUSED_EMPTY_POOL, CE_EventScript_WonderTrade_EmptyPool
	msgbox CE_Text_WonderTrade_Hello, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CE_EventScript_WonderTrade_Bye
CE_EventScript_WonderTrade_ChooseMon:
	msgbox CE_Text_WonderTrade_WhichMon, MSGBOX_DEFAULT
	chooseboxmon SELECT_PC_MON_NORMAL
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, CE_EventScript_WonderTrade_Bye
	callnative CeWonderTrade_ScriptCheckChosen, requests_effects=1
	switch VAR_RESULT
	case CE_WT_EGG, CE_EventScript_WonderTrade_Egg
	case CE_WT_CANT_TRADE, CE_EventScript_WonderTrade_CantTrade
	case CE_WT_DEAD, CE_EventScript_WonderTrade_CantTrade
	case CE_WT_MAIL, CE_EventScript_WonderTrade_Mail
	case CE_WT_BAG_FULL, CE_EventScript_WonderTrade_BagFull
	case CE_WT_LAST_SURFER, CE_EventScript_WonderTrade_LastSurfer
	case CE_WT_LAST_MON, CE_EventScript_WonderTrade_LastMon
	msgbox CE_Text_WonderTrade_Confirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CE_EventScript_WonderTrade_ChooseMon
	callnative CeWonderTrade_ScriptPrepare, requests_effects=1
	goto_if_eq VAR_RESULT, 0, CE_EventScript_WonderTrade_EmptyPool
	call_if_eq VAR_RESULT, 2, CE_EventScript_WonderTrade_ItemToBag
	msgbox CE_Text_WonderTrade_Sending, MSGBOX_DEFAULT
	special DoInGameTradeScene
	lock
	faceplayer
	callnative CeWonderTrade_ScriptFinish, requests_effects=1
	goto_if_eq VAR_RESULT, TRUE, CE_EventScript_WonderTrade_DonePc
	msgbox CE_Text_WonderTrade_Done, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_Again
	end

CE_EventScript_WonderTrade_DonePc:
	msgbox CE_Text_WonderTrade_DonePc, MSGBOX_DEFAULT
CE_EventScript_WonderTrade_Again:
	msgbox CE_Text_WonderTrade_Again, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, CE_EventScript_WonderTrade_ChooseMon
CE_EventScript_WonderTrade_Bye:
	msgbox CE_Text_WonderTrade_Bye, MSGBOX_DEFAULT
	release
	end

CE_EventScript_WonderTrade_ItemToBag:
	msgbox CE_Text_WonderTrade_ItemToBag, MSGBOX_DEFAULT
	return

CE_EventScript_WonderTrade_Off:
	msgbox CE_Text_WonderTrade_Off, MSGBOX_DEFAULT
	release
	end

CE_EventScript_WonderTrade_Rules:
	msgbox CE_Text_WonderTrade_Rules, MSGBOX_DEFAULT
	release
	end

CE_EventScript_WonderTrade_StaticTeam:
	msgbox CE_Text_WonderTrade_StaticTeam, MSGBOX_DEFAULT
	release
	end

CE_EventScript_WonderTrade_EmptyPool:
	msgbox CE_Text_WonderTrade_EmptyPool, MSGBOX_DEFAULT
	release
	end

CE_EventScript_WonderTrade_Egg:
	msgbox CE_Text_WonderTrade_Egg, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_EventScript_WonderTrade_CantTrade:
	msgbox CE_Text_WonderTrade_CantTrade, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_EventScript_WonderTrade_Mail:
	msgbox CE_Text_WonderTrade_Mail, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_EventScript_WonderTrade_BagFull:
	msgbox CE_Text_WonderTrade_BagFull, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_EventScript_WonderTrade_LastSurfer:
	msgbox CE_Text_WonderTrade_LastSurfer, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_EventScript_WonderTrade_LastMon:
	msgbox CE_Text_WonderTrade_LastMon, MSGBOX_DEFAULT
	goto CE_EventScript_WonderTrade_ChooseMon
	end

CE_Text_WonderTrade_Hello:
	.string "Hello! This is WONDER TRADE.\p"
	.string "Send a POKéMON from your party\n"
	.string "or PC, and a trainer somewhere\l"
	.string "sends you one back!\p"
	.string "It comes at the same level, but\n"
	.string "you never know which one it is.\p"
	.string "It's free! Want to trade?$"

CE_Text_WonderTrade_WhichMon:
	.string "Which POKéMON will you send?$"

CE_Text_WonderTrade_Confirm:
	.string "Send {STR_VAR_1} into\n"
	.string "WONDER TRADE?$"

CE_Text_WonderTrade_ItemToBag:
	.string "First, the {STR_VAR_2} it held\n"
	.string "goes back into your BAG.$"

CE_Text_WonderTrade_Sending:
	.string "Off it goes!$"

CE_Text_WonderTrade_Done:
	.string "{STR_VAR_2} sent you {STR_VAR_1}!\n"
	.string "Take good care of it!$"

CE_Text_WonderTrade_DonePc:
	.string "{STR_VAR_2} sent you {STR_VAR_1}!\p"
	.string "{STR_VAR_1} went to\n"
	.string "{STR_VAR_3} in the PC.$"

CE_Text_WonderTrade_Again:
	.string "Would you like to trade again?$"

CE_Text_WonderTrade_Bye:
	.string "Come back any time!$"

CE_Text_WonderTrade_Off:
	.string "WONDER TRADE is taking a break\n"
	.string "right now. Sorry!$"

CE_Text_WonderTrade_Rules:
	.string "Your challenge rules don't\n"
	.string "allow WONDER TRADE. Sorry!$"

CE_Text_WonderTrade_StaticTeam:
	.string "Your static team can't take in\n"
	.string "new POKéMON. Sorry!$"

CE_Text_WonderTrade_EmptyPool:
	.string "Nobody has a POKéMON of your\n"
	.string "type to trade now. Sorry!$"

CE_Text_WonderTrade_Egg:
	.string "An EGG can't go into\n"
	.string "WONDER TRADE. Hatch it first!$"

CE_Text_WonderTrade_CantTrade:
	.string "{STR_VAR_1} can't be traded.$"

CE_Text_WonderTrade_Mail:
	.string "{STR_VAR_1} is holding MAIL.\n"
	.string "Please take it off first.$"

CE_Text_WonderTrade_BagFull:
	.string "Your BAG has no room for the\n"
	.string "{STR_VAR_2} {STR_VAR_1} holds.$"

CE_Text_WonderTrade_LastSurfer:
	.string "{STR_VAR_1} is the last of your\n"
	.string "POKéMON that knows SURF!$"

CE_Text_WonderTrade_LastMon:
	.string "You can't send your last\n"
	.string "POKéMON that can battle!$"
