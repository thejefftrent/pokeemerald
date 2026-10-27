@ Sellers upstairs in every Pokémon Center (src/ce/progression.c): eight object events at the end
@ of the map.json of each of the 17 LAYOUT_POKEMON_CENTER_2F maps, which
@ CeProgression_ScriptUpdatePcSellers (called from CableClub_OnTransition) hides unless their
@ feature is on. Hide flags change only on a map load, so each script also asks
@ CeProgression_ScriptPcSellerOpen with its hide flag when talked to: a seller whose option the
@ player switched off in the menu closes instead of selling. The shop sellers each sell one of the
@ gCeShop lists whole; the items keep their own prices. tools/ce/gen_field_items.py does not list
@ these marts (it reads only scripts pulled in with .include, and the lists are C arrays), so the
@ mart editor and mart randomizer of the website leave them alone. Local ids differ between maps (Mauville and Mossdeep have one more
@ NPC), so these scripts never name their object. (cpp reads this file: no apostrophes here.)

CE_EventScript_PcSellerTraining::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_5
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerTraining
	waitmessage
	pokemart gCeShopTrainingItems
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_PcSellerEvolutionItems::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_6
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerEvolutionItems
	waitmessage
	pokemart gCeShopEvolutionItems
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_PcSellerMegaStones::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_7
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerMegaStones
	waitmessage
	pokemart gCeShopMegaStones
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_PcSellerZCrystals::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_8
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerZCrystals
	waitmessage
	pokemart gCeShopZCrystals
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_PcSellerGemsPlatesMemories::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_9
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerGemsPlatesMemories
	waitmessage
	pokemart gCeShopGemsPlatesMemories
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_PcSellerBattleItems::
	lock
	faceplayer
	callnative CeProgression_ScriptPcSellerOpen, requests_effects=1
	.2byte FLAG_TEMP_A
	goto_if_eq VAR_RESULT, FALSE, CE_EventScript_PcSellerClosed
	message CE_Text_PcSellerBattleItems
	waitmessage
	pokemart gCeShopBattleItems
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

@ A seller still standing after the player switched its option off.
CE_EventScript_PcSellerClosed:
	msgbox CE_Text_PcSellerClosed, MSGBOX_DEFAULT
	release
	end

@ The move tutor (CE_EventScript_PcSellerRelearner) is in data/scripts/ce/relearner.inc, and the
@ Wonder Trade attendant (CE_EventScript_PcSellerWonderTrade) in data/scripts/ce/wonder_trade.inc.

CE_Text_PcSellerClosed:
	.string "Sorry, my shop is closed\n"
	.string "right now.$"

CE_Text_PcSellerTraining:
	.string "Mints, vitamins, POWER items\n"
	.string "and ABILITY CAPSULES, too!$"

CE_Text_PcSellerEvolutionItems:
	.string "Evolution stones, trade items\n"
	.string "and a LINKING CORD, all here!$"

CE_Text_PcSellerMegaStones:
	.string "Hold a MEGA STONE and carry\n"
	.string "a MEGA RING to MEGA EVOLVE!$"

CE_Text_PcSellerZCrystals:
	.string "Hold a Z-CRYSTAL and carry a\n"
	.string "Z-POWER RING for a Z-MOVE!$"

CE_Text_PcSellerGemsPlatesMemories:
	.string "GEMS, PLATES and MEMORIES\n"
	.string "for every type of POKéMON!$"

CE_Text_PcSellerBattleItems:
	.string "Hold items for battle, from\n"
	.string "LEFTOVERS to a LIFE ORB!$"
