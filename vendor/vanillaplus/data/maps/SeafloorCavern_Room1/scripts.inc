SeafloorCavern_Room1_MapScripts::
	.byte 0

SeafloorCavern_Room1_EventScript_Grunt1::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_1, SeafloorCavern_Room1_Text_Grunt1Intro, SeafloorCavern_Room1_Text_Grunt1Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt1PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_EventScript_Grunt2::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_2, SeafloorCavern_Room1_Text_Grunt2Intro, SeafloorCavern_Room1_Text_Grunt2Defeat
	msgbox SeafloorCavern_Room1_Text_Grunt2PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room1_Text_Grunt1Intro:
	.string "Etrafta bir çocuğa ihtiyacımız yok!\nEvine git artık!$"

SeafloorCavern_Room1_Text_Grunt1Defeat:
	.string "Eve gitmek istiyorum.$"

SeafloorCavern_Room1_Text_Grunt1PostBattle:
	.string "Terfi almak istiyorum, böylece\nhaydutlara patronluk yapabilirim.$"

SeafloorCavern_Room1_Text_Grunt2Intro:
	.string "Şu denizaltının… İçi çok küçük.\nHer yerim ağrıyor!$"

SeafloorCavern_Room1_Text_Grunt2Defeat:
	.string "Kaybetmek beni üzüyor!$"

SeafloorCavern_Room1_Text_Grunt2PostBattle:
	.string "Çaldığımız denizaltı, adamım, zor\nbir yolculuktu. İçerisi çok dar!$"


