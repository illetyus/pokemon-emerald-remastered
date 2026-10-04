.set LOCALID_WALLACE, 1

CaveOfOrigin_B1F_MapScripts::
	.byte 0

CaveOfOrigin_B1F_EventScript_Wallace::
	lock
	faceplayer
	msgbox CaveOfOrigin_B1F_Text_WallaceStory, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_WALLACE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	delay 60
	playse SE_PIN
	applymovement LOCALID_WALLACE, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_WALLACE, Common_Movement_Delay48
	waitmovement 0
	delay 30
	applymovement LOCALID_WALLACE, Common_Movement_FacePlayer
	waitmovement 0
	message CaveOfOrigin_B1F_Text_WhereIsRayquaza
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_WheresRayquaza::
	multichoice 0, 0, MULTI_WHERES_RAYQUAZA, FALSE
	switch VAR_RESULT
	case 0, CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin
	case 1, CaveOfOrigin_B1F_EventScript_AtMtPyre
	case 2, CaveOfOrigin_B1F_EventScript_AtSkyPillar
	case 3, CaveOfOrigin_B1F_EventScript_DontRemember
	goto CaveOfOrigin_B1F_EventScript_DontRemember
	end

CaveOfOrigin_B1F_EventScript_AtCaveOfOrigin::
	message CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtMtPyre::
	message CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_DontRemember::
	message CaveOfOrigin_B1F_Text_CantYouRememberSomehow
	waitmessage
	goto CaveOfOrigin_B1F_EventScript_WheresRayquaza

CaveOfOrigin_B1F_EventScript_AtSkyPillar::
	msgbox CaveOfOrigin_B1F_Text_WellHeadToSkyPillar, MSGBOX_DEFAULT
	closemessage
	playse SE_EXIT
	fadescreenspeed FADE_TO_BLACK, 4
	setflag FLAG_WALLACE_GOES_TO_SKY_PILLAR
	setvar VAR_SOOTOPOLIS_CITY_STATE, 3
	removeobject LOCALID_WALLACE
	clearflag FLAG_HIDE_SKY_PILLAR_WALLACE
	fadescreen FADE_FROM_BLACK
	release
	end

CaveOfOrigin_B1F_Text_WallaceStory:
	.string "Ah, demek {PLAYER}{KUN} sensin.\nSenin kahramanlık hikayelerini\lduydum.\pBenim adım WALLACE.\pBir zamanlar SOOTOPOLİS'in SALON\nLİDERİ'ydim ama bir işim çıktı.\pŞimdi, SALON işlerini akıl hocam\nJUAN'a emanet ettim.\p… … … … … …\n… … … … … …\pBurada ortalığı kasıp kavuran iki\nPOKéMON'un, yani GROUDON & KYOGRE'ın\lsüper-kadim POKéMON oldukları\ldüşünülüyor.\pAma sadece iki süper-kadim POKEMON\nyok.\pBir yerlerde bir tane daha var.\pBir yerlerde, RAYQUAZA adında\nsüper-kadim bir POKéMON var.\pUzak geçmişte bu iki savaşçıyı\nboğanın RAYQUAZA olduğu söyleniyor.\pAma benim bile RAYQUAZA'nın nerede\nolduğuna dair hiçbir fikrim yok…$"

CaveOfOrigin_B1F_Text_WhereIsRayquaza:
	.string "WALLACE: {PLAYER}{KUN}, acaba\nRAYQUAZA'nın şu anda nerede olduğunu\lbiliyor musun?\pEğer biliyorsan, lütfen söyle.$"

CaveOfOrigin_B1F_Text_ButWereInCaveOfOrigin:
	.string "WALLACE: KÖKEN MAĞARASI mı?\pAma orası tam burası!\nBundan daha iyisini yapmanı\listiyorum!\pLütfen, RAYQUAZA'nın şu anda nerede\nolabileceğini düşünmeni istiyorum.$"

CaveOfOrigin_B1F_Text_OldLadyDidntMentionThat:
	.string "WALLACE: PYRE DAĞI mı?\pAma daha önce orada yaşlı kadınla\ngörüştüğümde bundan hiç\lbahsetmemişti.\pYaşlı kadının benden bir şey saklamaya\nçalışacağından çok şüpheliyim…\p{PLAYER}{KUN}, bunu benim için daha\ndikkatli düşünebilir misin?$"

CaveOfOrigin_B1F_Text_CantYouRememberSomehow:
	.string "WALLACE: Ha?\nHatırlamıyor musun?\lHmm… Bu bir sorun…\pBir şekilde hatırlayamaz mısın?$"

CaveOfOrigin_B1F_Text_WellHeadToSkyPillar:
	.string "WALLACE: GÖK SÜTUNU mu?\pİşte bu!\nGÖK SÜTUNU olmalı!\p{PLAYER}{KUN}, kaybedecek bir an bile yok!\nHemen GÖK SÜTUNU'na gidiyoruz!$"


