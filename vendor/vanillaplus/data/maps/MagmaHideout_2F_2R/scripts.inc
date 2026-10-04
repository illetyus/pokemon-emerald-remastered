MagmaHideout_2F_2R_MapScripts::
	.byte 0

MagmaHideout_2F_2R_EventScript_Grunt15::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_15, MagmaHideout_2F_2R_Text_Grunt15Intro, MagmaHideout_2F_2R_Text_Grunt15Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt15PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt6::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_6, MagmaHideout_2F_2R_Text_Grunt6Intro, MagmaHideout_2F_2R_Text_Grunt6Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt6PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt7::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_7, MagmaHideout_2F_2R_Text_Grunt7Intro, MagmaHideout_2F_2R_Text_Grunt7Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt7PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_EventScript_Grunt8::
	trainerbattle_single TRAINER_GRUNT_MAGMA_HIDEOUT_8, MagmaHideout_2F_2R_Text_Grunt8Intro, MagmaHideout_2F_2R_Text_Grunt8Defeat
	msgbox MagmaHideout_2F_2R_Text_Grunt8PostBattle, MSGBOX_AUTOCLOSE
	end

MagmaHideout_2F_2R_Text_Grunt15Intro:
	.string "Seninle bir alıp veremediğim yok.\nSadece emirleri uyguluyorum!$"

MagmaHideout_2F_2R_Text_Grunt15Defeat:
	.string "Kaybetmiş olabilirim.$"

MagmaHideout_2F_2R_Text_Grunt15PostBattle:
	.string "İnanılmaz bir şey bulduk!\nMAVİ KÜRE bizim!\pGeriye kalan tek şey liderimizin…\nFufufu… Fahahaha!$"

MagmaHideout_2F_2R_Text_Grunt6Intro:
	.string "Sıcağa dayanamıyorum.\nBelki de AQUA TAKIMI'na katılmalıydım.$"

MagmaHideout_2F_2R_Text_Grunt6Defeat:
	.string "Evet, MAGMA TAKIMI için doğru kişi\nolmayabilirim.$"

MagmaHideout_2F_2R_Text_Grunt6PostBattle:
	.string "Böyle bir yerde denizin hissettiği\nyalnızlığı düşünebiliyor musun?$"

MagmaHideout_2F_2R_Text_Grunt7Intro:
	.string "Ara sıra sarsıntılar duyabilirsin.\nAcaba bunlar volkan sarsıntısı mı?\p…Yoksa GROU…\nHoppala!\pHayır, hayır, boş ver!$"

MagmaHideout_2F_2R_Text_Grunt7Defeat:
	.string "Sen ateşli bir savaşçısın.\nTıpkı bir volkan gibi!$"

MagmaHideout_2F_2R_Text_Grunt7PostBattle:
	.string "Bu sefer kazandın ama bu pek önemli\ndeğil.\pMAGMA TAKIMI, hedefine ulaşmak üzere!$"

MagmaHideout_2F_2R_Text_Grunt8Intro:
	.string "Adamlarımızdan biri MAGMA amblemini\nkaybettiği için çıldırıyordu.\pBekle bir dakika!\nOnu alan sen miydin?$"

MagmaHideout_2F_2R_Text_Grunt8Defeat:
	.string "Buna inanmakta güçlük çekiyorum.$"

MagmaHideout_2F_2R_Text_Grunt8PostBattle:
	.string "Planımızın başarısız olacağı\nhissine kapılıyorum.$"


