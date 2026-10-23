@ Sellers in the player's room (CE_QOL_ROOM_SELLERS, src/ce/progression.c): one object event per
@ seller at the end of both bedrooms' map.json, which CeProgression_ScriptUpdateRoomSellers
@ hides unless its feature is on. Each sells one of the gCeShop lists whole; the items keep
@ their own prices. tools/ce/gen_field_items.py doesn't list these marts (it reads only
@ .include'd scripts, and the lists are C arrays), so the website's mart editor and mart
@ randomizer leave them alone.

CE_EventScript_RoomSellerMints::
	lock
	faceplayer
	message CE_Text_RoomSellerMints
	waitmessage
	pokemart gCeShopNatureMints
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_RoomSellerEvolutionItems::
	lock
	faceplayer
	message CE_Text_RoomSellerEvolutionItems
	waitmessage
	pokemart gCeShopEvolutionItems
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_RoomSellerMegaStones::
	lock
	faceplayer
	message CE_Text_RoomSellerMegaStones
	waitmessage
	pokemart gCeShopMegaStones
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_RoomSellerZCrystals::
	lock
	faceplayer
	message CE_Text_RoomSellerZCrystals
	waitmessage
	pokemart gCeShopZCrystals
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_EventScript_RoomSellerGemsPlatesMemories::
	lock
	faceplayer
	message CE_Text_RoomSellerGemsPlatesMemories
	waitmessage
	pokemart gCeShopGemsPlatesMemories
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

CE_Text_RoomSellerMints:
	.string "A mint gives a POKéMON's stats\n"
	.string "the boost of another nature.$"

CE_Text_RoomSellerEvolutionItems:
	.string "Evolution stones, trade items\n"
	.string "and a LINKING CORD, all here!$"

CE_Text_RoomSellerMegaStones:
	.string "Hold a MEGA STONE and carry\n"
	.string "a MEGA RING to MEGA EVOLVE!$"

CE_Text_RoomSellerZCrystals:
	.string "Hold a Z-CRYSTAL and carry a\n"
	.string "Z-POWER RING for a Z-MOVE!$"

CE_Text_RoomSellerGemsPlatesMemories:
	.string "GEMS, PLATES and MEMORIES\n"
	.string "for every type of POKéMON!$"
