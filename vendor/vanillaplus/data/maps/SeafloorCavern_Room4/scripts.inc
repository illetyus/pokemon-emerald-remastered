SeafloorCavern_Room4_MapScripts::
	.byte 0

SeafloorCavern_Room4_EventScript_Grunt3::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_3, SeafloorCavern_Room4_Text_Grunt3Intro, SeafloorCavern_Room4_Text_Grunt3Defeat
	msgbox SeafloorCavern_Room4_Text_Grunt3PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room4_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_4, SeafloorCavern_Room4_Text_Grunt4Intro, SeafloorCavern_Room4_Text_Grunt4Defeat
	msgbox SeafloorCavern_Room4_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room4_Text_Grunt3Intro:
	.string "Kimsin sen?\nNereden geldin?$"

SeafloorCavern_Room4_Text_Grunt3Defeat:
	.string "Kaybettim.$"

SeafloorCavern_Room4_Text_Grunt3PostBattle:
	.string "Çıkış yolunu bulamıyorum!\pKorkmuyorum. Beni yanlış anlama!$"

SeafloorCavern_Room4_Text_Grunt4Intro:
	.string "Kimsin sen?\nNereye gittiğini sanıyorsun?$"

SeafloorCavern_Room4_Text_Grunt4Defeat:
	.string "Kazanamadım!$"

SeafloorCavern_Room4_Text_Grunt4PostBattle:
	.string "Ortağım haritayı denizaltında unuttu!\pİşe yaramazlık değil de ne bu?$"


