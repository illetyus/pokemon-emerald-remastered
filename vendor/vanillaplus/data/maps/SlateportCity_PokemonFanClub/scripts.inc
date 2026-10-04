SlateportCity_PokemonFanClub_MapScripts::
	.byte 0

SlateportCity_PokemonFanClub_EventScript_Chairman::
	lock
	faceplayer
	goto_if_unset FLAG_ENTERED_CONTEST, SlateportCity_PokemonFanClub_EventScript_ChairmanNotEnteredContest
	call_if_unset FLAG_MET_SLATEPORT_FANCLUB_CHAIRMAN, SlateportCity_PokemonFanClub_EventScript_MeetChairman
	switch VAR_SLATEPORT_FAN_CLUB_STATE
	case 0, SlateportCity_PokemonFanClub_EventScript_ChairmanFirstAssessment
	case 1, SlateportCity_PokemonFanClub_EventScript_ChairmanTryAssessPokemon
	case 2, SlateportCity_PokemonFanClub_EventScript_NoMoreScarves
	release
	end

SlateportCity_PokemonFanClub_EventScript_ChairmanFirstAssessment::
	setvar VAR_SLATEPORT_FAN_CLUB_STATE, 1
	msgbox SlateportCity_PokemonFanClub_Text_AllowMeToExamineYourPokemon, MSGBOX_DEFAULT
	goto SlateportCity_PokemonFanClub_EventScript_ChairmanAssessLeadMon
	end

SlateportCity_PokemonFanClub_EventScript_ChairmanTryAssessPokemon::
	setvar VAR_TEMP_2, 0
	call_if_set FLAG_RECEIVED_YELLOW_SCARF, SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf
	call_if_set FLAG_RECEIVED_GREEN_SCARF, SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf
	call_if_set FLAG_RECEIVED_PINK_SCARF, SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf
	call_if_set FLAG_RECEIVED_BLUE_SCARF, SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf
	call_if_set FLAG_RECEIVED_RED_SCARF, SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf
	call_if_eq VAR_TEMP_2, CONTEST_CATEGORIES_COUNT, SlateportCity_PokemonFanClub_EventScript_ReceivedAllScarves
	goto_if_eq VAR_SLATEPORT_FAN_CLUB_STATE, 2, SlateportCity_PokemonFanClub_EventScript_NoMoreScarves
	msgbox SlateportCity_PokemonFanClub_Text_HowIsYourPokemonGrowing, MSGBOX_DEFAULT
	goto SlateportCity_PokemonFanClub_EventScript_ChairmanAssessLeadMon
	end

SlateportCity_PokemonFanClub_EventScript_NoMoreScarves::
	msgbox SlateportCity_PokemonFanClub_Text_NothingElseToGiveYou, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_ChairmanAssessLeadMon::
	msgbox SlateportCity_PokemonFanClub_Text_HmHmISee, MSGBOX_DEFAULT
	setvar VAR_TEMP_1, 0
	call_if_unset FLAG_RECEIVED_YELLOW_SCARF, SlateportCity_PokemonFanClub_EventScript_CheckMonTough
	call_if_unset FLAG_RECEIVED_GREEN_SCARF, SlateportCity_PokemonFanClub_EventScript_CheckMonSmart
	call_if_unset FLAG_RECEIVED_PINK_SCARF, SlateportCity_PokemonFanClub_EventScript_CheckMonCute
	call_if_unset FLAG_RECEIVED_BLUE_SCARF, SlateportCity_PokemonFanClub_EventScript_CheckMonBeauty
	call_if_unset FLAG_RECEIVED_RED_SCARF, SlateportCity_PokemonFanClub_EventScript_CheckMonCool
	bufferleadmonspeciesname STR_VAR_1
	switch VAR_TEMP_1
	case 0, SlateportCity_PokemonFanClub_EventScript_NoHighConditions
	case 1, SlateportCity_PokemonFanClub_EventScript_GiveRedScarf
	case 2, SlateportCity_PokemonFanClub_EventScript_GiveBlueScarf
	case 3, SlateportCity_PokemonFanClub_EventScript_GivePinkScarf
	case 4, SlateportCity_PokemonFanClub_EventScript_GiveGreenScarf
	case 5, SlateportCity_PokemonFanClub_EventScript_GiveYellowScarf
	release
	end

SlateportCity_PokemonFanClub_EventScript_ReceivedAllScarves::
	setvar VAR_SLATEPORT_FAN_CLUB_STATE, 2
	return

SlateportCity_PokemonFanClub_EventScript_CountReceivedScarf::
	addvar VAR_TEMP_2, 1
	return

SlateportCity_PokemonFanClub_EventScript_NoHighConditions::
	msgbox SlateportCity_PokemonFanClub_Text_GiveMonMorePokeblocks, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_GiveRedScarf::
	checkitemspace ITEM_RED_SCARF
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf
	msgbox SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_RED_SCARF
	giveitem ITEM_RED_SCARF
	msgbox SlateportCity_PokemonFanClub_Text_ExplainRedScarf, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_GiveBlueScarf::
	checkitemspace ITEM_BLUE_SCARF
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf
	msgbox SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_BLUE_SCARF
	giveitem ITEM_BLUE_SCARF
	msgbox SlateportCity_PokemonFanClub_Text_ExplainBlueScarf, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_GivePinkScarf::
	checkitemspace ITEM_PINK_SCARF
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf
	msgbox SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_PINK_SCARF
	giveitem ITEM_PINK_SCARF
	msgbox SlateportCity_PokemonFanClub_Text_ExplainPinkScarf, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_GiveGreenScarf::
	checkitemspace ITEM_GREEN_SCARF
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf
	msgbox SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_GREEN_SCARF
	giveitem ITEM_GREEN_SCARF
	msgbox SlateportCity_PokemonFanClub_Text_ExplainGreenScarf, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_GiveYellowScarf::
	checkitemspace ITEM_YELLOW_SCARF
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf
	msgbox SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis, MSGBOX_DEFAULT
	setflag FLAG_RECEIVED_YELLOW_SCARF
	giveitem ITEM_YELLOW_SCARF
	msgbox SlateportCity_PokemonFanClub_Text_ExplainYellowScarf, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_NoRoomForScarf::
	msgbox SlateportCity_PokemonFanClub_Text_NoSpaceForReward, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_CheckMonCool::
	specialvar VAR_RESULT, CheckLeadMonCool
	call_if_eq VAR_RESULT, TRUE, SlateportCity_PokemonFanClub_EventScript_SetMonCool
	return

SlateportCity_PokemonFanClub_EventScript_SetMonCool::
	setvar VAR_TEMP_1, 1
	return

SlateportCity_PokemonFanClub_EventScript_CheckMonBeauty::
	specialvar VAR_RESULT, CheckLeadMonBeauty
	call_if_eq VAR_RESULT, TRUE, SlateportCity_PokemonFanClub_EventScript_SetMonBeauty
	return

SlateportCity_PokemonFanClub_EventScript_SetMonBeauty::
	setvar VAR_TEMP_1, 2
	return

SlateportCity_PokemonFanClub_EventScript_CheckMonCute::
	specialvar VAR_RESULT, CheckLeadMonCute
	call_if_eq VAR_RESULT, TRUE, SlateportCity_PokemonFanClub_EventScript_SetMonCute
	return

SlateportCity_PokemonFanClub_EventScript_SetMonCute::
	setvar VAR_TEMP_1, 3
	return

SlateportCity_PokemonFanClub_EventScript_CheckMonSmart::
	specialvar VAR_RESULT, CheckLeadMonSmart
	call_if_eq VAR_RESULT, TRUE, SlateportCity_PokemonFanClub_EventScript_SetMonSmart
	return

SlateportCity_PokemonFanClub_EventScript_SetMonSmart::
	setvar VAR_TEMP_1, 4
	return

SlateportCity_PokemonFanClub_EventScript_CheckMonTough::
	specialvar VAR_RESULT, CheckLeadMonTough
	call_if_eq VAR_RESULT, TRUE, SlateportCity_PokemonFanClub_EventScript_SetMonTough
	return

SlateportCity_PokemonFanClub_EventScript_SetMonTough::
	setvar VAR_TEMP_1, 5
	return

SlateportCity_PokemonFanClub_EventScript_ChairmanNotEnteredContest::
	goto_if_set FLAG_MET_SLATEPORT_FANCLUB_CHAIRMAN, SlateportCity_PokemonFanClub_EventScript_ChairmanEnterContest
	msgbox SlateportCity_PokemonFanClub_Text_MeetChairman, MSGBOX_DEFAULT
	setflag FLAG_MET_SLATEPORT_FANCLUB_CHAIRMAN
	release
	end

SlateportCity_PokemonFanClub_EventScript_ChairmanEnterContest::
	msgbox SlateportCity_PokemonFanClub_Text_LikeToSeeEnteredContestPokemon, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_MeetChairman::
	msgbox SlateportCity_PokemonFanClub_Text_MeetChairman, MSGBOX_DEFAULT
	setflag FLAG_MET_SLATEPORT_FANCLUB_CHAIRMAN
	return

SlateportCity_PokemonFanClub_EventScript_SootheBellWoman::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_SOOTHE_BELL, SlateportCity_PokemonFanClub_EventScript_ReceivedSootheBell
	msgbox SlateportCity_PokemonFanClub_Text_ShowMePokemonThatLoveYou, MSGBOX_DEFAULT
	specialvar VAR_RESULT, GetLeadMonFriendshipScore
	goto_if_ge VAR_RESULT, FRIENDSHIP_150_TO_199, SlateportCity_PokemonFanClub_EventScript_GiveSootheBell
	release
	end

SlateportCity_PokemonFanClub_EventScript_GiveSootheBell::
	playse SE_PIN
	applymovement VAR_LAST_TALKED, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement VAR_LAST_TALKED, Common_Movement_Delay48
	waitmovement 0
	msgbox SlateportCity_PokemonFanClub_Text_PokemonAdoresYou, MSGBOX_DEFAULT
	giveitem ITEM_SOOTHE_BELL
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_SOOTHE_BELL
	release
	end

SlateportCity_PokemonFanClub_EventScript_ReceivedSootheBell::
	msgbox SlateportCity_PokemonFanClub_Text_TreatPokemonWithLove, MSGBOX_DEFAULT
	release
	end

SlateportCity_PokemonFanClub_EventScript_Man::
	msgbox SlateportCity_PokemonFanClub_Text_PokemonDontLikeFainting, MSGBOX_NPC
	end

SlateportCity_PokemonFanClub_EventScript_Twin::
	msgbox SlateportCity_PokemonFanClub_Text_MonEnjoyedProtein, MSGBOX_NPC
	end

SlateportCity_PokemonFanClub_EventScript_Skitty::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_SKITTY, CRY_MODE_NORMAL
	msgbox SlateportCity_PokemonFanClub_Text_Skitty, MSGBOX_DEFAULT
	waitmoncry
	release
	end

SlateportCity_PokemonFanClub_EventScript_Zigzagoon::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_ZIGZAGOON, CRY_MODE_NORMAL
	msgbox SlateportCity_PokemonFanClub_Text_Zigzagoon, MSGBOX_DEFAULT
	waitmoncry
	release
	end

SlateportCity_PokemonFanClub_EventScript_Azumarill::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZUMARILL, CRY_MODE_NORMAL
	msgbox SlateportCity_PokemonFanClub_Text_Azumarill, MSGBOX_DEFAULT
	waitmoncry
	release
	end

SlateportCity_PokemonFanClub_Text_MeetChairman:
	.string "E-hem! Ben POKéMON FAN KULÜBÜ\nbakşanıyım!\pBaşkan olduğum için buradaki en\nönemli kişi de benim!\pKonu POKéMON yetiştirmek olduğunda\nkimse beni geçemez. Hiç kimse!\pSana POKéMON YARIŞMALARI'ndan\nbahsedeyim.\pPOKéMON'ları tüm dünyanın görmesi için\nsergileyebileceğin etkinliklerdir!\pAncak, uzak şehirlerde\ndüzenleniyorlar, bu yüzden yeterince\lsık katılamıyorum.\pBu yüzden burada POKéMON'umuzu\ngöstermek ve başkalarının da bize\lkendi POKéMON'larını göstermelerini\lsağlamak için toplanıyoruz.$"

SlateportCity_PokemonFanClub_Text_LikeToSeeEnteredContestPokemon:
	.string "POKéMON YARIŞMASINA katılmış bir\neğitmenin POKéMON'u…\lBunu görmeyi çok isterdim.$"

SlateportCity_PokemonFanClub_Text_AllowMeToExamineYourPokemon:
	.string "E-hem! Bir POKéMON yarışmasına\nkatılmışsın sanırım!\pLütfen! POKéMON'unu nasıl\nyetiştirdiğini incelememe izin ver.\pDiğer EĞİTMENLER tarafından\nyetiştirilen POKéMON'ları görmek beni\lson derece memnun ediyor.\pÖzenle yetiştirilen POKéMON'ların\nmutlu ifadeleri…\pPOKéMON'u besleyen ve büyüten\neğitmenin şefkatli gözleri…\pBu düşünce bile kalbimi sevinçle\ndolduruyor.\pAh! Kusura bakma! Böyle gevezelik\nettiğim için beni affet!\pLütfen! POKéMON'unun ne kadar\nbüyüdüğünü görmeme izin ver!$"

SlateportCity_PokemonFanClub_Text_HowIsYourPokemonGrowing:
	.string "POKéMON'un nasıl durumda?\nBir inceleyeyim.$"

SlateportCity_PokemonFanClub_Text_HmHmISee:
	.string "Hm, hm…\nAnlıyorum…$"

SlateportCity_PokemonFanClub_Text_GiveMonMorePokeblocks:
	.string "Hmmm…\nKötü değil, ama iyi de değil…\pBu POKéMON'u yetiştirmek için\ndaha fazla çaba göstermelisin!\pÖrneğin, ona daha fazla {POKEBLOCK}\nvermeni önerebilir miyim?$"

SlateportCity_PokemonFanClub_Text_NoSpaceForReward:
	.string "Aman Tanrım.\pPOKéMON'un oldukça iyi büyüyor, bu\nyüzden bir ödülü hak ediyorsun.\pNe yazık ki, çantanda bunun için\nyer yok.$"

SlateportCity_PokemonFanClub_Text_MonMostImpressiveGiveItThis:
	.string "Şu {STR_VAR_1} çok etkileyici\nbir şekilde büyüyor!\lÇok güzel bir örnek!\pAma! Eğer bunu ona verirsen, daha da\niyi büyüyecektir!\lGerçekten de öyle!$"

SlateportCity_PokemonFanClub_Text_ExplainRedScarf:
	.string "POKéMON'una o kırmızı atkıyı ver.\pHerkes o POKéMON'un havalı olduğunu\nfark edecek!$"

SlateportCity_PokemonFanClub_Text_ExplainBlueScarf:
	.string "POKéMON'una o mavi atkıyı ver.\pGüzelliği şimdi olduğundan çok daha\nfazla dikkat çekecek!$"

SlateportCity_PokemonFanClub_Text_ExplainPinkScarf:
	.string "POKéMON'una o pembe atkıyı ver.\pBu, POKéMON'un sevimliliğini biraz daha\nortaya çıkaracaktır!$"

SlateportCity_PokemonFanClub_Text_ExplainGreenScarf:
	.string "POKéMON'una o yeşil atkıyı ver.\pBu, POKéMON'un zekasını artıracaktır!$"

SlateportCity_PokemonFanClub_Text_ExplainYellowScarf:
	.string "POKéMON'una o sarı atkıyı ver.\pPOKéMON'unun dayanıklılığını çok\ndaha fazla destekleyecektir!$"

SlateportCity_PokemonFanClub_Text_NothingElseToGiveYou:
	.string "Üzgünüm, ama sana verecek başka bir\nşeyim kalmadı! Hem de hiç!\pNe de olsa, herhangi bir eşyaya\nbaşvurmadan POKéMON yetiştirme\lyeteneğiyle kutsanmışsın!$"

SlateportCity_PokemonFanClub_Text_ShowMePokemonThatLoveYou:
	.string "Eğitmenlerini seven POKéMON'lar\ngörmeye bayılıyorum.\pPOKéMON'lar eğitmenlerinin\nduygularına karşı çok hassastır.\pEğer POKéMON'una sevgi ve özenle\nyaklaşırsan, o da seni sevecektir.\pPOKéMON'un seni sevmeye başladığında\nlütfen gelip bana göster.$"

SlateportCity_PokemonFanClub_Text_PokemonAdoresYou:
	.string "POKéMON'un seni gerçekten seviyor.\pBu senin için, şefkatli eğitmen.\nHAYRAN KULÜBÜ'nden bir hediye!$"

SlateportCity_PokemonFanClub_Text_TreatPokemonWithLove:
	.string "POKéMON'lar eğitmenlerinin duygularına\nkarşı çok hassastır.\pEğer POKéMON'una sevgi ve özen\ngösterirsen, onlar da seni sevecektir.$"

SlateportCity_PokemonFanClub_Text_PokemonDontLikeFainting:
	.string "Bir POKéMON'u savaşta sürekli\nbayıltırsan, buna içerlemeye\lbaşlayacaktır.\pYakında, eğitmenine daha az güvenir\nhale gelecektir.\pBaşka bir deyişle seni gittikçe\ndaha az sevecektir.$"

SlateportCity_PokemonFanClub_Text_MonEnjoyedProtein:
	.string "POKéMON'lar, üzerlerinde eşya\nkullanılmasından hoşlanır mı?\pBenimki ona biraz PROTEİN verdiğimde\ngerçekten mutlu oldu.$"

SlateportCity_PokemonFanClub_Text_Skitty:
	.string "SKİTTY: Fffnyaaaah…$"

SlateportCity_PokemonFanClub_Text_Zigzagoon:
	.string "ZİGZAGOON: Kyuuu…$"

SlateportCity_PokemonFanClub_Text_Azumarill:
	.string "AZUMARİLL: Marimari?$"


