@ QoL key items (src/ce/key_items.c starts these from the bag or the Select button).
@ Included by data/event_scripts.s.

@ The Fallarbor Move Tutor (FallarborTown_MoveRelearnersHouse) without the Heart Scale:
@ level-up moves, or with CE_QOL_RELEARNER_EGG_TUTOR the list the player picks (level-up, egg or
@ tutor moves, data/scripts/ce/relearner.inc), Pokemon from the party or the PC, repeated until
@ the player cancels.
CE_EventScript_MoveRelearnerItem::
	lockall
CE_EventScript_MoveRelearnerItem_ChooseMon:
	msgbox CE_Text_MoveRelearnerItem_WhichMon, MSGBOX_DEFAULT
	call CE_EventScript_RelearnerKeyItemList
	chooseboxmon SELECT_PC_MON_MOVE_RELEARNER
	waitstate
	setmoverelearnerstate VAR_0x8006
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, CE_EventScript_KeyItemDone
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, CE_EventScript_MoveRelearnerItem_Egg
	special Special_HasMoveToRelearn
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_MoveRelearnerItem_NoMove
	msgbox CE_Text_MoveRelearnerItem_WhichMove, MSGBOX_DEFAULT
	special TeachMoveRelearnerMove
	goto CE_EventScript_MoveRelearnerItem_ChooseMon
	end

CE_EventScript_MoveRelearnerItem_Egg:
	msgbox CE_Text_MoveRelearnerItem_Egg, MSGBOX_DEFAULT
	goto CE_EventScript_MoveRelearnerItem_ChooseMon
	end

CE_EventScript_MoveRelearnerItem_NoMove:
	msgbox CE_Text_MoveRelearnerItem_NoMove, MSGBOX_DEFAULT
	goto CE_EventScript_MoveRelearnerItem_ChooseMon
	end

@ The Lilycove Move Deleter (LilycoveCity_MoveDeletersHouse), the same specials in the same
@ order, repeated until the player cancels.
CE_EventScript_MoveDeleterItem::
	lockall
CE_EventScript_MoveDeleterItem_ChooseMon:
	msgbox CE_Text_MoveDeleterItem_WhichMon, MSGBOX_DEFAULT
	chooseboxmon SELECT_PC_MON_MOVE_DELETER
	waitstate
	goto_if_eq VAR_0x8004, PARTY_NOTHING_CHOSEN, CE_EventScript_KeyItemDone
	special IsSelectedMonEgg
	goto_if_eq VAR_RESULT, TRUE, CE_EventScript_MoveDeleterItem_Egg
	special GetNumMovesSelectedMonHas
	goto_if_eq VAR_RESULT, 1, CE_EventScript_MoveDeleterItem_OneMove
	msgbox CE_Text_MoveDeleterItem_WhichMove, MSGBOX_DEFAULT
	fadescreen FADE_TO_BLACK
	special MoveDeleterChooseMoveToForget
	fadescreen FADE_FROM_BLACK
	goto_if_eq VAR_0x8005, MAX_MON_MOVES, CE_EventScript_MoveDeleterItem_ChooseMon
	special BufferMoveDeleterNicknameAndMove
	msgbox CE_Text_MoveDeleterItem_Confirm, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, CE_EventScript_MoveDeleterItem_ChooseMon
	special IsLastMonThatKnowsSurf
	goto_if_eq VAR_RESULT, TRUE, CE_EventScript_MoveDeleterItem_LastSurf
	special MoveDeleterForgetMove
	playfanfare MUS_MOVE_DELETED
	waitfanfare
	msgbox CE_Text_MoveDeleterItem_Forgot, MSGBOX_DEFAULT
	goto CE_EventScript_MoveDeleterItem_ChooseMon
	end

CE_EventScript_MoveDeleterItem_Egg:
	msgbox CE_Text_MoveDeleterItem_Egg, MSGBOX_DEFAULT
	goto CE_EventScript_MoveDeleterItem_ChooseMon
	end

CE_EventScript_MoveDeleterItem_OneMove:
	special BufferMoveDeleterNicknameAndMove
	msgbox CE_Text_MoveDeleterItem_OneMove, MSGBOX_DEFAULT
	goto CE_EventScript_MoveDeleterItem_ChooseMon
	end

CE_EventScript_MoveDeleterItem_LastSurf:
	msgbox CE_Text_MoveDeleterItem_LastSurf, MSGBOX_DEFAULT
	goto CE_EventScript_MoveDeleterItem_ChooseMon
	end

CE_EventScript_KeyItemDone:
	releaseall
	end

CE_Text_MoveRelearnerItem_WhichMon:
	.string "Which POKéMON should remember\n"
	.string "a move?$"

CE_Text_MoveRelearnerItem_WhichMove:
	.string "Which move should it remember?$"

CE_Text_MoveRelearnerItem_NoMove:
	.string "There is no move that this POKéMON\n"
	.string "can remember.$"

CE_Text_MoveRelearnerItem_Egg:
	.string "An EGG has no moves to remember.$"

CE_Text_MoveDeleterItem_WhichMon:
	.string "Which POKéMON should forget a move?$"

CE_Text_MoveDeleterItem_WhichMove:
	.string "Which move should be forgotten?$"

CE_Text_MoveDeleterItem_Confirm:
	.string "Make {STR_VAR_1} forget\n"
	.string "{STR_VAR_2}?$"

CE_Text_MoveDeleterItem_Forgot:
	.string "{STR_VAR_1} has forgotten\n"
	.string "{STR_VAR_2}.$"

CE_Text_MoveDeleterItem_Egg:
	.string "An EGG has no moves to forget.$"

CE_Text_MoveDeleterItem_OneMove:
	.string "{STR_VAR_1} knows only one move,\n"
	.string "so it can't forget it.$"

CE_Text_MoveDeleterItem_LastSurf:
	.string "{STR_VAR_1} can't forget SURF.\n"
	.string "No other POKéMON knows it.$"
