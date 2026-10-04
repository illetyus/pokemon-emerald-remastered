RustboroCity_PokemonSchool_MapScripts::
	.byte 0

RustboroCity_PokemonSchool_EventScript_Blackboard::
	lockall
	msgbox RustboroCity_PokemonSchool_Text_BlackboardListsStatusChanges, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic::
	message RustboroCity_PokemonSchool_Text_ReadWhichTopic
	waitmessage
	multichoicegrid 8, 1, MULTI_STATUS_INFO, 3, FALSE
	switch VAR_RESULT
	case 0, RustboroCity_PokemonSchool_EventScript_Poison
	case 1, RustboroCity_PokemonSchool_EventScript_Paralysis
	case 2, RustboroCity_PokemonSchool_EventScript_Sleep
	case 3, RustboroCity_PokemonSchool_EventScript_Burn
	case 4, RustboroCity_PokemonSchool_EventScript_Freeze
	case 5, RustboroCity_PokemonSchool_EventScript_ExitTopicSelect
	case MULTI_B_PRESSED, RustboroCity_PokemonSchool_EventScript_ExitTopicSelect
	end

RustboroCity_PokemonSchool_EventScript_Poison::
	msgbox RustboroCity_PokemonSchool_Text_ExplainPoison, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Paralysis::
	msgbox RustboroCity_PokemonSchool_Text_ExplainParalysis, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Sleep::
	msgbox RustboroCity_PokemonSchool_Text_ExplainSleep, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Burn::
	msgbox RustboroCity_PokemonSchool_Text_ExplainBurn, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_Freeze::
	msgbox RustboroCity_PokemonSchool_Text_ExplainFreeze, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_ChooseBlackboardTopic
	end

RustboroCity_PokemonSchool_EventScript_ExitTopicSelect::
	releaseall
	end

RustboroCity_PokemonSchool_EventScript_GameboyKid1::
	msgbox RustboroCity_PokemonSchool_Text_TradingRightNow, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_GameboyKid2::
	msgbox RustboroCity_PokemonSchool_Text_AlwaysWantedSeedot, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_RichBoy::
	msgbox RustboroCity_PokemonSchool_Text_PokemontCantUseManMadeItems, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_Lass::
	msgbox RustboroCity_PokemonSchool_Text_ConfusedPokemonAttacksItself, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_SchoolKidM::
	msgbox RustboroCity_PokemonSchool_Text_PokemonHealItselfWithBerry, MSGBOX_NPC
	end

RustboroCity_PokemonSchool_EventScript_StudentNotebook::
	msgbox RustboroCity_PokemonSchool_Text_StudentsNotes, MSGBOX_SIGN
	end

RustboroCity_PokemonSchool_EventScript_Teacher::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_QUICK_CLAW, RustboroCity_PokemonSchool_EventScript_GaveQuickClaw
	call_if_eq VAR_FACING, DIR_EAST, RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsEast
	call_if_eq VAR_FACING, DIR_WEST, RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsWest
	msgbox RustboroCity_PokemonSchool_Text_StudentsWhoDontStudyGetQuickClaw, MSGBOX_DEFAULT
	giveitem ITEM_QUICK_CLAW
	goto_if_eq VAR_RESULT, 0, Common_EventScript_ShowBagIsFull
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	setflag FLAG_RECEIVED_QUICK_CLAW
	release
	end

RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsEast::
	applymovement VAR_LAST_TALKED, RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsEast
	waitmovement 0
	return

RustboroCity_PokemonSchool_EventScript_TeacherCheckOnStudentsWest::
	applymovement VAR_LAST_TALKED, RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsWest
	waitmovement 0
	return

RustboroCity_PokemonSchool_EventScript_GaveQuickClaw::
	msgbox RustboroCity_PokemonSchool_Text_ExplainQuickClaw, MSGBOX_DEFAULT
	closemessage
	applymovement VAR_LAST_TALKED, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	release
	end

RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsWest:
	walk_left
	walk_down
	walk_down
	walk_right
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	walk_down
	walk_in_place_faster_left
	delay_16
	delay_16
	walk_right
	delay_16
	delay_16
	delay_8
	walk_up
	walk_left
	walk_left
	walk_up
	walk_up
	walk_right
	step_end

RustboroCity_PokemonSchool_Movement_TeacherCheckOnStudentsEast:
	walk_right
	walk_right
	walk_down
	walk_down
	walk_left
	walk_left
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	walk_down
	walk_in_place_faster_left
	delay_16
	delay_16
	walk_right
	delay_16
	delay_16
	delay_8
	walk_up
	walk_right
	walk_up
	walk_up
	walk_left
	walk_left
	step_end

RustboroCity_PokemonSchool_EventScript_Scott::
	lock
	faceplayer
	goto_if_set FLAG_MET_SCOTT_AFTER_OBTAINING_STONE_BADGE, RustboroCity_PokemonSchool_EventScript_ScottWatchStudents
	goto_if_set FLAG_MET_SCOTT_RUSTBORO, RustboroCity_PokemonSchool_EventScript_ScottSpokeAlready
	goto_if_set FLAG_BADGE01_GET, RustboroCity_PokemonSchool_EventScript_ScottGreetHasBadge
	msgbox RustboroCity_PokemonSchool_Text_ScottMetAlreadyCut, MSGBOX_DEFAULT
	addvar VAR_SCOTT_STATE, 1
	setflag FLAG_MET_SCOTT_RUSTBORO
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottSpokeAlready::
	goto_if_set FLAG_BADGE01_GET, RustboroCity_PokemonSchool_EventScript_ScottNoticeBadge
	msgbox RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown, MSGBOX_DEFAULT
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottGreetHasBadge::
	msgbox RustboroCity_PokemonSchool_Text_ScottMetAlreadyStoneBadge, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge
	end

RustboroCity_PokemonSchool_EventScript_ScottNoticeBadge::
	msgbox RustboroCity_PokemonSchool_Text_ScottStoneBadge, MSGBOX_DEFAULT
	goto RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge
	end

RustboroCity_PokemonSchool_EventScript_MetScottAfterBadge::
	addvar VAR_SCOTT_STATE, 1
	setflag FLAG_MET_SCOTT_AFTER_OBTAINING_STONE_BADGE
	release
	end

RustboroCity_PokemonSchool_EventScript_ScottWatchStudents::
	msgbox RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown, MSGBOX_DEFAULT
	release
	end

RustboroCity_PokemonSchool_Text_BlackboardListsStatusChanges:
	.string "Yazı tahtasında, savaşta POKéMON'u\netkileyebilecek durum değişiklikleri\lyazıyor.$"

RustboroCity_PokemonSchool_Text_ReadWhichTopic:
	.string "Hangi konuyu okumak istersin?$"

RustboroCity_PokemonSchool_Text_ExplainPoison:
	.string "Eğer bir POKéMON zehirlenirse, sürekli\nolarak can kaybedecektir.\pZehrin etkileri savaştan sonra da devam\neder.\pZehirlenmiş bir POKéMON'un canı\nyürürken de düşer.\pPANZEHİR kullanarak zehirlenmeyi\ngeçirebilirsiniz.$"

RustboroCity_PokemonSchool_Text_ExplainParalysis:
	.string "Eğer bir POKéMON felç olursa, HIZI\ndüşer.\pAyrıca savaştayken hareket\nedemeyebilir.\pFelç, savaştan sonra da devam eder.\nBir FELÇ İLACI kullanarak\liyileştirebilirsiniz.$"

RustboroCity_PokemonSchool_Text_ExplainSleep:
	.string "Eğer bir POKéMON uykuya dalarsa,\nsaldıramaz.\pBir POKéMON kendi kendine uyanabilir,\nancak uyurken bir savaş sona ererse,\luykuda kalacaktır.\pBir UYANDIRICI kullanarak onu\nuyandırabilirsiniz.$"

RustboroCity_PokemonSchool_Text_ExplainBurn:
	.string "Yanık, SALDIRI gücünü azaltır ve\nkurbanın canını sürekli olarak\ldüşürür.\pYanık savaştan sonra da devam eder.\nYANIK İYİLEŞTİRME kullanarak bir yanığı\liyileştirebilirsiniz.$"

RustboroCity_PokemonSchool_Text_ExplainFreeze:
	.string "Eğer bir POKéMON donarsa, tamamen\nçaresiz hale gelir.\pSavaştan sonra donmuş olarak\nkalacaktır. BUZ ÇÖZÜCÜ kullanarak\lçözebilirsiniz.$"

RustboroCity_PokemonSchool_Text_StudentsWhoDontStudyGetQuickClaw:
	.string "Ders çalışmayan öğrenciler HIZLI\nPENÇE'min tadına bakarlar.\pİyi bir öğrenci olup olmadığın, bunu\nkullanma şeklinden anlaşılacaktır.$"

RustboroCity_PokemonSchool_Text_ExplainQuickClaw:
	.string "HIZLI PENÇE'yi tutan bir POKéMON\nzaman zaman hızlanır ve rakibinden önce\lsaldırır.\pPOKéMON tarafından tutulması gereken\ndaha birçok eşya vardır.\pSadece bunlar bile çalışacak birçok\nkonu verecektir!$"

RustboroCity_PokemonSchool_Text_TradingRightNow:
	.string "Şu anda arkadaşımla POKéMON takası\nyapıyorum.$"

RustboroCity_PokemonSchool_Text_AlwaysWantedSeedot:
	.string "Hep bir SEEDOT istemiştim ve sonunda\nbirini buldum!$"

RustboroCity_PokemonSchool_Text_PokemontCantUseManMadeItems:
	.string "POKéMON eşya tutabilir, ancak\nİKSİR ve PANZEHİR gibi insan yapımı\leşyalarla ne yapacaklarını bilmiyorlar.$"

RustboroCity_PokemonSchool_Text_ConfusedPokemonAttacksItself:
	.string "Bazı POKéMON saldırılarının,\nPOKéMON'un kafasını karıştırabileceğini\lbiliyor musun?\pKafası karışmış bir POKéMON bazen\nistemeden kendine saldırır.\pAncak savaşı terk ettiğinde normale\ndönecektir.$"

RustboroCity_PokemonSchool_Text_PokemonHealItselfWithBerry:
	.string "MEYVE tutan bir POKéMON kendini\niyileştirebilir…\pPOKéMON'un tutabileceği pek çok çeşit\neşya vardır…\pNot almak gerçekten çok zor.$"

RustboroCity_PokemonSchool_Text_StudentsNotes:
	.string "Bu bir öğrencinin not defteri.\pPOKéMON, POKé TOPLARI kullanılarak\nyakalanmalıdır.\pBir EĞİTMEN'e en fazla altı POKéMON\neşlik edebilir.\pEĞİTMEN, POKéMON yakalayan, onları\nyetiştiren ve onlarla savaşan kişidir.\pEğitmenin görevi, POKéMON\nSALON'larında meydan okuyucuları\lbekleyen güçlü eğitmenleri yenmektir.$"

RustboroCity_PokemonSchool_Text_ScottMetAlreadyCut:
	.string "Selam?\nDaha önce görüşmemiş miydik?\pSanırım PETALBURG ŞEHRİ'nde seni\ngörmüştüm.\pKendimi tanıtayım.\nBenim adım SCOTT.\pSeçkin eğitmenler bulmak için her yeri\ndolaşıyorum.\pDaha spesifik olarak, POKéMON savaş\nuzmanları arıyorum.\pPeki, seni bu okula getiren nedir?\nSen de bir EĞİTMEN misin?\pYapman gereken ilk şey,\nPOKéMON'unun KESME saldırısını\löğrenmesini sağlamak.\pEğer doğru hatırlıyorsam, bu kasabadan\nbirinde KESME HM'si vardı.$"

RustboroCity_PokemonSchool_Text_StudentTalentLevelUnknown:
	.string "SCOTT: Hmm…\pBuradaki öğrencilerin yetenek\nseviyeleri bilinmiyor. Potansiyel var.$"

RustboroCity_PokemonSchool_Text_ScottStoneBadge:
	.string "SCOTT: O da ne?\pBu bir TAŞ ROZET, değil mi? Oldukça\netkileyici olduğunu söyleyebilirim.\pAma, bilirsin, seni savaşta görmeyi çok\nisterdim.\pBir EĞİTMEN olarak nasıl biri olduğunu\nLİG ROZETİ'nden anlamak zor.$"

RustboroCity_PokemonSchool_Text_ScottMetAlreadyStoneBadge:
	.string "Selam?\nDaha önce görüşmemiş miydik?\pSanırım PETALBURG ŞEHRİ'nde seni\ngörmüştüm.\pKendimi tanıtayım.\nBenim adım SCOTT.\pSeçkin eğitmenler bulmak için her yeri\ndolaşıyorum.\pDaha spesifik olarak, POKéMON savaş\nuzmanları arıyorum.\pO da ne? Bu bir taş rozet, değil mi?\pSenin gibi yeni başlayan bir eğitmen için\noldukça etkileyici olduğunu\lsöyleyebilirim.\pAma seni savaşta görmeyi çok isterdim.\pBir EĞİTMEN olarak nasıl biri\nolduğunu LİG ROZETİ'nden anlamak\lzor.$"


