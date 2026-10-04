DewfordTown_Gym_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, DewfordTown_Gym_OnTransition
	.byte 0

DewfordTown_Gym_OnTransition:
	call DewfordTown_Gym_EventScript_SetFlashLevel
	end

DewfordTown_Gym_EventScript_SetFlashLevel::
	goto_if_defeated TRAINER_BRAWLY_1, DewfordTown_Gym_EventScript_SetLightsOn
	call DewfordTown_Gym_EventScript_CountTrainersDefeated
	copyvar VAR_0x8001, VAR_0x8000
	goto_if_eq VAR_0x8000, 0, DewfordTown_Gym_EventScript_SetFlashLevel7
	goto_if_eq VAR_0x8000, 1, DewfordTown_Gym_EventScript_SetFlashLevel6
	goto_if_eq VAR_0x8000, 2, DewfordTown_Gym_EventScript_SetFlashLevel5
	goto_if_eq VAR_0x8000, 3, DewfordTown_Gym_EventScript_SetFlashLevel4
	goto_if_eq VAR_0x8000, 4, DewfordTown_Gym_EventScript_SetFlashLevel3
	goto_if_eq VAR_0x8000, 5, DewfordTown_Gym_EventScript_SetFlashLevel2
	goto DewfordTown_Gym_EventScript_SetFlashLevel1

DewfordTown_Gym_EventScript_SetLightsOn::
	setflashlevel 0
	return

@ Brightest
DewfordTown_Gym_EventScript_SetFlashLevel1::
	setflashlevel 1
	return

DewfordTown_Gym_EventScript_SetFlashLevel2::
	setflashlevel 2
	return

DewfordTown_Gym_EventScript_SetFlashLevel3::
	setflashlevel 3
	return

DewfordTown_Gym_EventScript_SetFlashLevel4::
	setflashlevel 4
	return

DewfordTown_Gym_EventScript_SetFlashLevel5::
	setflashlevel 5
	return

DewfordTown_Gym_EventScript_SetFlashLevel6::
	setflashlevel 6
	return

@ Darkest
DewfordTown_Gym_EventScript_SetFlashLevel7::
	setflashlevel 7
	return

DewfordTown_Gym_EventScript_BrightenRoom::
	call DewfordTown_Gym_EventScript_CountTrainersDefeated
	nop1
	goto_if_eq VAR_0x8000, VAR_0x8001, DewfordTown_Gym_EventScript_NoLightChange
	copyvar VAR_0x8001, VAR_0x8000
	goto_if_eq VAR_0x8000, 1, DewfordTown_Gym_EventScript_AnimateFlash1Trainer
	goto_if_eq VAR_0x8000, 2, DewfordTown_Gym_EventScript_AnimateFlash2Trainers
	goto_if_eq VAR_0x8000, 3, DewfordTown_Gym_EventScript_AnimateFlash3Trainers
	goto_if_eq VAR_0x8000, 4, DewfordTown_Gym_EventScript_AnimateFlash4Trainers
	goto_if_eq VAR_0x8000, 5, DewfordTown_Gym_EventScript_AnimateFlash5Trainers
	goto_if_eq VAR_0x8000, 6, DewfordTown_Gym_EventScript_AnimateFlash6Trainers

DewfordTown_Gym_EventScript_NoLightChange::
	return

@ NOTE: A little confusingly, a larger animateflash param value is a smaller flash radius
DewfordTown_Gym_EventScript_AnimateFlash1Trainer::
	playse SE_SWITCH
	animateflash 6
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash2Trainers::
	playse SE_SWITCH
	animateflash 5
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash3Trainers::
	playse SE_SWITCH
	animateflash 4
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash4Trainers::
	playse SE_SWITCH
	animateflash 3
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash5Trainers::
	playse SE_SWITCH
	animateflash 2
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlash6Trainers::
	playse SE_SWITCH
	animateflash 1
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_AnimateFlashFullBrightness::
	playse SE_SWITCH
	animateflash 0
	call DewfordTown_Gym_EventScript_SetFlashLevel
	return

DewfordTown_Gym_EventScript_CountTrainersDefeated::
	setvar VAR_0x8000, 0
	goto_if_not_defeated TRAINER_TAKAO, DewfordTown_Gym_EventScript_CheckJocelyn
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckJocelyn::
	goto_if_not_defeated TRAINER_JOCELYN, DewfordTown_Gym_EventScript_CheckLaura
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckLaura::
	goto_if_not_defeated TRAINER_LAURA, DewfordTown_Gym_EventScript_CheckBrenden
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckBrenden::
	goto_if_not_defeated TRAINER_BRENDEN, DewfordTown_Gym_EventScript_CheckCristian
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckCristian::
	goto_if_not_defeated TRAINER_CRISTIAN, DewfordTown_Gym_EventScript_CheckLilith
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_CheckLilith::
	goto_if_not_defeated TRAINER_LILITH, DewfordTown_Gym_EventScript_StopCountingTrainers
	addvar VAR_0x8000, 1
DewfordTown_Gym_EventScript_StopCountingTrainers::
	return

DewfordTown_Gym_EventScript_Brawly::
	trainerbattle_single TRAINER_BRAWLY_1, DewfordTown_Gym_Text_BrawlyIntro, DewfordTown_Gym_Text_BrawlyDefeat, DewfordTown_Gym_EventScript_BrawlyDefeated, NO_MUSIC
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, DewfordTown_Gym_EventScript_BrawlyRematch
	goto_if_unset FLAG_RECEIVED_TM08, DewfordTown_Gym_EventScript_GiveBulkUp2
	msgbox DewfordTown_Gym_Text_BrawlyPostBattle, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_BrawlyDefeated::
	call DewfordTown_Gym_EventScript_AnimateFlashFullBrightness
	message DewfordTown_Gym_Text_ReceivedKnuckleBadge
	waitmessage
	call Common_EventScript_PlayGymBadgeFanfare
	msgbox DewfordTown_Gym_Text_KnuckleBadgeInfoTakeThis, MSGBOX_DEFAULT
	setflag FLAG_DEFEATED_DEWFORD_GYM
	setflag FLAG_BADGE02_GET
	addvar VAR_PETALBURG_GYM_STATE, 1
	call_if_eq VAR_PETALBURG_GYM_STATE, 6, Common_EventScript_ReadyPetalburgGymForBattle
	setvar VAR_0x8008, 2
	call Common_EventScript_SetGymTrainers
	call DewfordTown_Gym_EventScript_GiveBulkUp
	closemessage
	delay 30
	playfanfare MUS_REGISTER_MATCH_CALL
	msgbox DewfordTown_Gym_Text_RegisteredBrawly, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	delay 30
	setflag FLAG_ENABLE_BRAWLY_MATCH_CALL
	setvar VAR_ROXANNE_CALL_STEP_COUNTER, 0
	setflag FLAG_ENABLE_ROXANNE_FIRST_CALL
	release
	end

DewfordTown_Gym_EventScript_GiveBulkUp::
	giveitem ITEM_TM08
	goto_if_eq VAR_RESULT, 0, Common_EventScript_BagIsFull
	msgbox DewfordTown_Gym_Text_ExplainBulkUp, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM08
	return

DewfordTown_Gym_EventScript_GiveBulkUp2:
	giveitem ITEM_TM08
	goto_if_eq VAR_RESULT, 0, Common_EventScript_ShowBagIsFull
	msgbox DewfordTown_Gym_Text_ExplainBulkUp, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_TM08
	release
	end

DewfordTown_Gym_EventScript_BrawlyRematch::
	trainerbattle_rematch_double TRAINER_BRAWLY_1, DewfordTown_Gym_Text_BrawlyPreRematch, DewfordTown_Gym_Text_BrawlyRematchDefeat, DewfordTown_Gym_Text_BrawlyRematchNeedTwoMons
	msgbox DewfordTown_Gym_Text_BrawlyPostRematch, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_Takao::
	trainerbattle_single TRAINER_TAKAO, DewfordTown_Gym_Text_TakaoIntro, DewfordTown_Gym_Text_TakaoDefeat, DewfordTown_Gym_EventScript_TakaoBrightenRoom
	msgbox DewfordTown_Gym_Text_TakaoPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_TakaoBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Jocelyn::
	trainerbattle_single TRAINER_JOCELYN, DewfordTown_Gym_Text_JocelynIntro, DewfordTown_Gym_Text_JocelynDefeat, DewfordTown_Gym_EventScript_JocelynBrightenRoom
	msgbox DewfordTown_Gym_Text_JocelynPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_JocelynBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Laura::
	trainerbattle_single TRAINER_LAURA, DewfordTown_Gym_Text_LauraIntro, DewfordTown_Gym_Text_LauraDefeat, DewfordTown_Gym_EventScript_LauraBrightenRoom
	msgbox DewfordTown_Gym_Text_LauraPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_LauraBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Brenden::
	trainerbattle_single TRAINER_BRENDEN, DewfordTown_Gym_Text_BrendenIntro, DewfordTown_Gym_Text_BrendenDefeat, DewfordTown_Gym_EventScript_BrendenBrightenRoom
	msgbox DewfordTown_Gym_Text_BrendenPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_BrendenBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Cristian::
	trainerbattle_single TRAINER_CRISTIAN, DewfordTown_Gym_Text_CristianIntro, DewfordTown_Gym_Text_CristianDefeat, DewfordTown_Gym_EventScript_CristianBrightenRoom
	msgbox DewfordTown_Gym_Text_CristianPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_CristianBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_Lilith::
	trainerbattle_single TRAINER_LILITH, DewfordTown_Gym_Text_LilithIntro, DewfordTown_Gym_Text_LilithDefeat, DewfordTown_Gym_EventScript_LilithBrightenRoom
	msgbox DewfordTown_Gym_Text_LilithPostBattle, MSGBOX_AUTOCLOSE
	end

DewfordTown_Gym_EventScript_LilithBrightenRoom::
	call DewfordTown_Gym_EventScript_BrightenRoom
	release
	end

DewfordTown_Gym_EventScript_GymGuide::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_DEWFORD_GYM, DewfordTown_Gym_EventScript_GymGuidePostVictory
	msgbox DewfordTown_Gym_Text_GymGuideAdvice, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_GymGuidePostVictory::
	msgbox DewfordTown_Gym_Text_GymGuidePostVictory, MSGBOX_DEFAULT
	release
	end

DewfordTown_Gym_EventScript_LeftGymStatue::
	lockall
	goto_if_set FLAG_BADGE02_GET, DewfordTown_Gym_EventScript_GymStatueCertified
	goto DewfordTown_Gym_EventScript_GymStatue
	end

DewfordTown_Gym_EventScript_RightGymStatue::
	lockall
	goto_if_set FLAG_BADGE02_GET, DewfordTown_Gym_EventScript_GymStatueCertified
	goto DewfordTown_Gym_EventScript_GymStatue
	end

DewfordTown_Gym_EventScript_GymStatueCertified::
	msgbox DewfordTown_Gym_Text_GymStatueCertified, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Gym_EventScript_GymStatue::
	msgbox DewfordTown_Gym_Text_GymStatue, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Gym_Text_GymGuideAdvice:
	.string "Hey, nasıl gidiyor, ŞAMPİYON adayı\n{PLAYER}?\pDEWFORD'un SALON LİDERİ, DÖVÜŞ tipi\nPOKéMON'lar kullanıyor.\pOna NORMAL tip POKéMON ile karşı çıkmak\nbela aramaktır.\pDÖVÜŞ POKéMON'ları, NORMAL POKéMON'lara\nciddi bir darbe indirecektir, bu yüzden\ldikkatli ol!\pDaha da kötüsü bu SALON, okyanus\ntabanı kadar karanlık.\pAncak yoluna çıkan eğitmenleri\nyendikten sonra daha aydınlık olacak.\pHadi, devam et!$"

DewfordTown_Gym_Text_GymGuidePostVictory:
	.string "Vay be!\nBurası çok aydınlık oldu!\pAynı senin geleceğin gibi!$"

DewfordTown_Gym_Text_TakaoIntro:
	.string "Güçlü olan haklıdır!\nHaydi!$"

DewfordTown_Gym_Text_TakaoDefeat:
	.string "Yeterli gücüm yok.$"

DewfordTown_Gym_Text_TakaoPostBattle:
	.string "Yeteneğin benim gücümü aştı!\nTekniğin övgüye değer!$"

DewfordTown_Gym_Text_JocelynIntro:
	.string "BRAWLY'in karışmasına gerek yok!\pSeni ezeceğim!$"

DewfordTown_Gym_Text_JocelynDefeat:
	.string "Ama… Nasıl? Nasıl bu kadar kolay\nkaybedebildim?$"

DewfordTown_Gym_Text_JocelynPostBattle:
	.string "Bu kaybı boşa harcamayacağım.\nBununla motivasyon tazeleyeceğim!\pBir gün SALON LİDERİ olacağım.\pHayır, ben POKéMON LİGİ ŞAMPİYONU\nolacağım!$"

DewfordTown_Gym_Text_LauraIntro:
	.string "Sakın beni başından savmaya kalkma!\nAğlarsan bu benim suçum değil!$"

DewfordTown_Gym_Text_LauraDefeat:
	.string "Vaaaaaaah!\nKaybettim!$"

DewfordTown_Gym_Text_LauraPostBattle:
	.string "Kalbimi güçlendirmek için karanlıkta\nsavaşıyorum.\pAma çok karanlık olduğu için BRAWLY'nin\nyüzünü göremiyorum…$"

DewfordTown_Gym_Text_LilithIntro:
	.string "Ufufu…\nKaranlıkta tökezliyor musun?$"

DewfordTown_Gym_Text_LilithDefeat:
	.string "S-sen…\nSeni görmek istemiyorum!$"

DewfordTown_Gym_Text_LilithPostBattle:
	.string "Kaybolmayı ve tekrar girişe dönmeyi\nhak ediyorsun!$"

DewfordTown_Gym_Text_BrendenIntro:
	.string "Sana bir denizcinin cesaretini\ngöstereceğim!$"

DewfordTown_Gym_Text_BrendenDefeat:
	.string "Bu nasıl oldu?\nBöyle kaybetmek bana göre değil!$"

DewfordTown_Gym_Text_BrendenPostBattle:
	.string "Peki!\nSALON LİDERİMİZİ görmeye layıksın.$"

DewfordTown_Gym_Text_CristianIntro:
	.string "Eğer rozet istiyorsan, ilk\nbeni geçmen gerekecek!$"

DewfordTown_Gym_Text_CristianDefeat:
	.string "Grrrrr!\nAşırı güçlüsün!$"

DewfordTown_Gym_Text_CristianPostBattle:
	.string "Sen kazandın!\nO rozetin peşinden git!$"

DewfordTown_Gym_Text_BrawlyIntro:
	.string "Ben BRAWLY'im!\nDEWFORD'un SALON LİDERİ!\pBu bölgelerin sert dalgalarında\nçalkalandım ve zifiri karanlık mağarada\lgüçlendim!\pBana meydan okumak mı istiyorsun?\nNeye benzediğini görmeme izin ver!$"

DewfordTown_Gym_Text_BrawlyDefeat:
	.string "Vay canına!\nBeklediğimden çok daha iyiydin!\pBeni batırdın!\pTamam, al bu SALON rozetini!$"

DewfordTown_Gym_Text_ReceivedKnuckleBadge:
	.string "{PLAYER}, BRAWLY'den YUMRUK ROZETİ\naldı.$"

DewfordTown_Gym_Text_KnuckleBadgeInfoTakeThis:
	.string "YUMRUK ROZETİ, 30. Seviyeye kadar olan\ntüm POKéMON'ların, hatta takasla\laldıklarının bile sorgusuz sualsiz\litaat etmesini sağlar.\pAyrıca, HM saldırısı FLASH'ı savaş\ndışında da kullanabileceksin.\pİstersen bu TEKNİK MAKİNEYİ de\nkullanabilirsin.$"

DewfordTown_Gym_Text_ExplainBulkUp:
	.string "TM08'de BULK UP var.\pHem SALDIRI hem de SAVUNMA\ndeğerlerini yükselten bir saldırı.\p… … … … … …$"

DewfordTown_Gym_Text_RegisteredBrawly:
	.string "GYM LİDERİ BRAWLY, POKéNAV'a\nkaydedildi.$"

DewfordTown_Gym_Text_BrawlyPostBattle:
	.string "Yeteneğinin bir gün eğitmenler\narasında şaşkınlık fırtınası yaratacak\ldev bir dalgaya dönüştüğünü\lgörebiliyorum!$"

DewfordTown_Gym_Text_GymStatue:
	.string "DEWFORD KASABASI POKéMON SALONU$"

DewfordTown_Gym_Text_GymStatueCertified:
	.string "DEWFORD KASBASI POKéMON SALONU\pBRAWLY'NİN ONAYLI EĞİTMENLERİ:\n{PLAYER}$"

DewfordTown_Gym_Text_BrawlyPreRematch:
	.string "BRAWLY: Bir dalga geri çekilebilir, ama\nher zaman kıyıya geri döner.\pSenin gibi dev bir yetenek dalgası…\nGeri döneceğini biliyordum!\pNe kadar yükseldiğini göster bana!$"

DewfordTown_Gym_Text_BrawlyRematchDefeat:
	.string "Vay canına!\nYine battık!$"

DewfordTown_Gym_Text_BrawlyPostRematch:
	.string "Savaş uçsuz bucaksız ve derindir.\nDibini göremezsin.\lTıpkı HOENN'deki denizler gibi!$"

DewfordTown_Gym_Text_BrawlyRematchNeedTwoMons:
	.string "BRAWLY: Bir dalga geri çekilebilir, ama\nher zaman kıyıya geri döner.\pSenin gibi yetenekli dev bir dalga…\nGeri döneceğini biliyordum!\pÖyle mi? Savaşabilecek sadece bir\nPOKéMON'un var.\pÜzgünüm ama en az iki POKéMON'la\ndönmeni istiyorum.$"
