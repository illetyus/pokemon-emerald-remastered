MagmaHideout_3F_1R_MapScripts::
	.byte 0

MagmaHideout_3F_1R_EventScript_Grunt9::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_9, MagmaHideout_3F_1R_Text_Grunt9Intro, MagmaHideout_3F_1R_Text_Grunt9Defeat
	msgbox MagmaHideout_3F_1R_Text_Grunt9PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_3F_1R_EventScript_Grunt16::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_16, MagmaHideout_3F_1R_Text_Grunt16Intro, MagmaHideout_3F_1R_Text_Grunt16Defeat
	msgbox MagmaHideout_3F_1R_Text_Grunt16PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_3F_1R_Text_Grunt9Intro:
	.string "Bu nöbetçi işini hak edecek ne yaptım ben?\pSol kulağım yanıyor!$"

MagmaHideout_3F_1R_Text_Grunt9Defeat:
	.string "Sıcaktan bayılacağım.$"

MagmaHideout_3F_1R_Text_Grunt9PostBattle:
	.string "Magma dolu bu volkanda şapka\ntakmamız sence de garip değil mi?$"

MagmaHideout_3F_1R_Text_Grunt16Intro:
	.string "Liderimizin muhteşem vizyonuna\nulaşmasına yardımcı olabilmek için\lkatıldık.\pAQUA TAKIMI'ndan olman ya da yoldan\ngeçen bir çocuk olman umurumda\ldeğil.\pKimse bize karışamaz ve bundan paçayı\nkurtaramaz!$"

MagmaHideout_3F_1R_Text_Grunt16Defeat:
	.string "Hayır!\nSana hiç güven olmaz!$"

MagmaHideout_3F_1R_Text_Grunt16PostBattle:
	.string "Beni iyi dinle.\nMAGMA TAKIMI haklı!\pAQUA takımını dinleme.\nYalanlarına inanma!$"


