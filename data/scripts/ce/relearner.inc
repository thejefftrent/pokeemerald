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
