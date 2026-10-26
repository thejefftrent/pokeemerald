@ The move relearner egg and tutor lists (include/ce/relearner.h, src/ce/relearner.c).
@ Included by data/event_scripts.s with cpp: no apostrophes in these comments.
@
@ The lists are upstream relearner states, set with setmoverelearnerstate before chooseboxmon:
@ MOVE_RELEARNER_LEVEL_UP_MOVES, MOVE_RELEARNER_EGG_MOVES (every stage of the evolution line)
@ and MOVE_RELEARNER_TUTOR_MOVES. The relearner screen runs them in script mode.

@ CE_QOL_RELEARNER_EGG_TUTOR. Called from the Fallarbor Town Move Relearner
@ (FallarborTown_MoveRelearnersHouse ChooseMon) where it set the level-up list: with the feature
@ off it still sets the level-up list; with it on the player picks the list. Each move taught
@ still costs a Heart Scale. Cancelling leaves the tutor as B in the party menu does.
CE_EventScript_RelearnerFallarborList::
	callnative CeRelearner_ScriptStartList, requests_effects=1
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_RelearnerListReturn
	call CE_EventScript_RelearnerAskLevelUpEggTutor
	goto_if_eq VAR_RESULT, FALSE, FallarborTown_MoveRelearnersHouse_EventScript_ComeBackWithHeartScale
CE_EventScript_RelearnerListReturn:
	return

@ The same for the Move Relearner key item (data/scripts/ce/key_items.inc), which stays free.
CE_EventScript_RelearnerKeyItemList::
	callnative CeRelearner_ScriptStartList, requests_effects=1
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_RelearnerListReturn
	call CE_EventScript_RelearnerAskLevelUpEggTutor
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_KeyItemDone
	return

@ Sets the list the player picks; VAR_RESULT is FALSE if the player cancelled.
CE_EventScript_RelearnerAskLevelUpEggTutor:
	message CE_Text_Relearner_WhichList
	waitmessage
	dynmultipush CE_Text_Relearner_LevelUpMoves, MOVE_RELEARNER_LEVEL_UP_MOVES
	dynmultipush CE_Text_Relearner_EggMoves, MOVE_RELEARNER_EGG_MOVES
	dynmultipush CE_Text_Relearner_TutorMoves, MOVE_RELEARNER_TUTOR_MOVES
	dynmultipush CE_Text_Relearner_Cancel, MULTI_B_PRESSED
	dynmultistack 0, 0, FALSE, 4, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	closemessage
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, CE_EventScript_RelearnerAskCancelled
	setmoverelearnerstate VAR_RESULT
	setvar VAR_RESULT, TRUE
	return

CE_EventScript_RelearnerAskCancelled:
	setvar VAR_RESULT, FALSE
	return

@ CE_QOL_TUTOR_SELLER: the move tutor upstairs in every Pokemon Center (the FLAG_TEMP_B seller of
@ sPcSellers, src/ce/progression.c; its object is at the end of each 2F map.json). Free and
@ unlimited: the player picks egg moves or tutor moves, then Pokemon from the party or the PC,
@ until cancelling the Pokemon goes back to the question and cancelling the question ends.
CE_EventScript_PcSellerRelearner::
	lockall
	faceplayer
	msgbox CE_Text_PcRelearner_Hello, MSGBOX_DEFAULT
CE_EventScript_PcRelearner_Ask:
	message CE_Text_Relearner_WhichList
	waitmessage
	dynmultipush CE_Text_Relearner_EggMoves, MOVE_RELEARNER_EGG_MOVES
	dynmultipush CE_Text_Relearner_TutorMoves, MOVE_RELEARNER_TUTOR_MOVES
	dynmultipush CE_Text_Relearner_Cancel, MULTI_B_PRESSED
	dynmultistack 0, 0, FALSE, 3, FALSE, 0, DYN_MULTICHOICE_CB_NONE
	closemessage
	goto_if_eq VAR_RESULT, MULTI_B_PRESSED, CE_EventScript_PcRelearner_Bye
	setmoverelearnerstate VAR_RESULT
CE_EventScript_PcRelearner_ChooseMon:
	msgbox CE_Text_PcRelearner_WhichMon, MSGBOX_DEFAULT
	chooseboxmon SELECT_PC_MON_MOVE_RELEARNER
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, CE_EventScript_PcRelearner_Ask
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, CE_EventScript_PcRelearner_Egg
	special Special_HasMoveToRelearn
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcRelearner_NoMove
	msgbox CE_Text_PcRelearner_WhichMove, MSGBOX_DEFAULT
	special TeachMoveRelearnerMove
	goto CE_EventScript_PcRelearner_ChooseMon
	end

CE_EventScript_PcRelearner_Egg:
	msgbox CE_Text_PcRelearner_Egg, MSGBOX_DEFAULT
	goto CE_EventScript_PcRelearner_ChooseMon
	end

CE_EventScript_PcRelearner_NoMove:
	msgbox CE_Text_PcRelearner_NoMove, MSGBOX_DEFAULT
	goto CE_EventScript_PcRelearner_ChooseMon
	end

CE_EventScript_PcRelearner_Bye:
	msgbox CE_Text_PcRelearner_Bye, MSGBOX_DEFAULT
	releaseall
	end

CE_Text_PcRelearner_Hello:
	.string "EGG moves or tutor moves, I teach\n"
	.string "them all, free of charge!$"

CE_Text_PcRelearner_WhichMon:
	.string "Which POKéMON should learn a move?$"

CE_Text_PcRelearner_WhichMove:
	.string "Which move should it learn?$"

CE_Text_PcRelearner_Egg:
	.string "An EGG can't learn any moves yet.$"

CE_Text_PcRelearner_NoMove:
	.string "I have no move of that kind\n"
	.string "to teach that POKéMON.$"

CE_Text_PcRelearner_Bye:
	.string "Come back any time!$"

CE_Text_Relearner_WhichList:
	.string "Which kind of move?$"

CE_Text_Relearner_LevelUpMoves:
	.string "LEVEL-UP MOVES$"

CE_Text_Relearner_EggMoves:
	.string "EGG MOVES$"

CE_Text_Relearner_TutorMoves:
	.string "TUTOR MOVES$"

CE_Text_Relearner_Cancel:
	.string "CANCEL$"
