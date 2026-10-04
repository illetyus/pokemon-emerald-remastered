RustboroCity_House3_MapScripts::
	.byte 0

RustboroCity_House3_EventScript_OldMan::
	msgbox RustboroCity_House3_Text_IGivePerfectlySuitedNicknames, MSGBOX_NPC
	end

RustboroCity_House3_EventScript_OldWoman::
	msgbox RustboroCity_House3_Text_NamingPikachuPekachu, MSGBOX_NPC
	end

@ Misspelling on purpose, see nickname
RustboroCity_House3_EventScript_Pekachu::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_PIKACHU, CRY_MODE_NORMAL
	msgbox RustboroCity_House3_Text_Pekachu, MSGBOX_DEFAULT
	waitmoncry
	release
	end

RustboroCity_House3_Text_IGivePerfectlySuitedNicknames:
	.string "POKéMON'larıma mükemmel takma\nadlar veririm!\pBu benim özgün tarafımı gösterir!$"

RustboroCity_House3_Text_NamingPikachuPekachu:
	.string "Ama PIKACHU'ya PEKACHU adını\nvermek mi? Çok anlamsız.\pSanırım anlaşılması kolay bir isim\nkullanmak iyi bir şey ama yine de…$"

RustboroCity_House3_Text_Pekachu:
	.string "PEKACHU: Peka!$"


