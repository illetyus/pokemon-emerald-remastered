MagmaHideout_2F_1R_MapScripts::
	.byte 0

MagmaHideout_2F_1R_EventScript_Grunt14::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_14, MagmaHideout_2F_1R_Text_Grunt14Intro, MagmaHideout_2F_1R_Text_Grunt14Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt14PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt3::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_3, MagmaHideout_2F_1R_Text_Grunt3Intro, MagmaHideout_2F_1R_Text_Grunt3Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt3PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt4::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_4, MagmaHideout_2F_1R_Text_Grunt4Intro, MagmaHideout_2F_1R_Text_Grunt4Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt4PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_EventScript_Grunt5::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_5, MagmaHideout_2F_1R_Text_Grunt5Intro, MagmaHideout_2F_1R_Text_Grunt5Defeat
	msgbox MagmaHideout_2F_1R_Text_Grunt5PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_1R_Text_Grunt14Intro:
	.string "Ne, ne, ne?\pBurada sadece MAGMA üyelerinin\nolması gerekiyor!\pMAGMA TAKIMI üniforması giymemişsin…\nÇok şüphe çekiyorsun!\pSavaş benimle!$"

MagmaHideout_2F_1R_Text_Grunt14Defeat:
	.string "Aiyiyiy…$"

MagmaHideout_2F_1R_Text_Grunt14PostBattle:
	.string "Eğer üşüyorsan, MAGMA TAKIMINA\nkatılmalısın.$"

MagmaHideout_2F_1R_Text_Grunt3Intro:
	.string "Orada dur bakalım!\pGerçekten yanımdan sessizce geçip\ngitmene izin vermemi beklemiyorsun,\ldeğil mi?$"

MagmaHideout_2F_1R_Text_Grunt3Defeat:
	.string "Vay canına!\nYenilgiyi kabulleniyorum!$"

MagmaHideout_2F_1R_Text_Grunt3PostBattle:
	.string "Belki de tartışmasız gitmene izin\nverseydim daha iyi olurdu…$"

MagmaHideout_2F_1R_Text_Grunt4Intro:
	.string "Ahah!\nDavetsiz misafir!$"

MagmaHideout_2F_1R_Text_Grunt4Defeat:
	.string "Graaah!$"

MagmaHideout_2F_1R_Text_Grunt4PostBattle:
	.string "Zaten kaybettim.\pGerçekten bir çemberin içinde koşmaya\ndevam etmek zorunda mıyım?$"

MagmaHideout_2F_1R_Text_Grunt5Intro:
	.string "Hey, hey!\nDavetsiz misafir!$"

MagmaHideout_2F_1R_Text_Grunt5Defeat:
	.string "Olamaz.$"

MagmaHideout_2F_1R_Text_Grunt5PostBattle:
	.string "Aslında, savaş konusunda uzman\ndeğilimdir.$"


