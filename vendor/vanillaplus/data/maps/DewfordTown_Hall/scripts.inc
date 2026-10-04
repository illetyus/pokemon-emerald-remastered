.set LOCALID_EXPERT_M, 4
.set LOCALID_TWIN, 5
.set LOCALID_SCHOOL_KID_M, 7
.set LOCALID_PSYCHIC_M, 8

DewfordTown_Hall_MapScripts::
	.byte 0

DewfordTown_Hall_EventScript_Girl::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	special IsTrendyPhraseBoring
	goto_if_eq VAR_RESULT, TRUE, DewfordTown_Hall_EventScript_GirlBoredOfTrend
	msgbox DewfordTown_Hall_Text_CantImagineLifeWithoutTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_GirlBoredOfTrend::
	msgbox DewfordTown_Hall_Text_GettingBoredOfTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Woman::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_TeachingMonAboutTrend, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Man::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	special BufferDeepLinkPhrase
	msgbox DewfordTown_Hall_Text_DeepLinkBetweenXAndY, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, DewfordTown_Hall_EventScript_ConfirmTrendLink
	goto_if_eq VAR_RESULT, NO, DewfordTown_Hall_EventScript_RejectTrendLink
	end

DewfordTown_Hall_EventScript_ConfirmTrendLink::
	msgbox DewfordTown_Hall_Text_MyHunchWasRight, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_RejectTrendLink::
	msgbox DewfordTown_Hall_Text_NotEasyToKeepUp, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_ExpertM::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_TVShowAboutTrend, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_EXPERT_M, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	release
	end

DewfordTown_Hall_EventScript_Twin::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_IsTrendMorePopularAcrossSea, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_TWIN, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	release
	end

DewfordTown_Hall_EventScript_LittleBoy::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_CollectTrendMerchandise, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_EventScript_Bookshelf::
	lockall
	call Common_EventScript_BufferTrendyPhrase
	msgbox DewfordTown_Hall_Text_BooksAboutTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_Painting::
	lockall
	call Common_EventScript_BufferTrendyPhrase
	special GetDewfordHallPaintingNameIndex
	switch VAR_RESULT
	case 0, DewfordTown_Hall_EventScript_ScreamTitle
	case 4, DewfordTown_Hall_EventScript_ScreamTitle
	case 1, DewfordTown_Hall_EventScript_SmileTitle
	case 5, DewfordTown_Hall_EventScript_ScreamTitle
	case 2, DewfordTown_Hall_EventScript_LastTitle
	case 6, DewfordTown_Hall_EventScript_LastTitle
	case 3, DewfordTown_Hall_EventScript_BirthTitle
	case 7, DewfordTown_Hall_EventScript_LastTitle
	end

DewfordTown_Hall_EventScript_ScreamTitle::
	msgbox DewfordTown_Hall_Text_TrendsScream, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_SmileTitle::
	msgbox DewfordTown_Hall_Text_TrendsSmile, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_LastTitle::
	msgbox DewfordTown_Hall_Text_LastTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_BirthTitle::
	msgbox DewfordTown_Hall_Text_BirthOfTrend, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_SchoolKidM::
	lockall
	setvar VAR_0x8008, 0
	goto DewfordTown_Hall_EventScript_DoTrendDebate
	end

DewfordTown_Hall_EventScript_PsychicM::
	lockall
	setvar VAR_0x8008, 1
	goto DewfordTown_Hall_EventScript_DoTrendDebate
	end

DewfordTown_Hall_EventScript_DoTrendDebate::
	call Common_EventScript_BufferTrendyPhrase
	special GetDewfordHallPaintingNameIndex
	switch VAR_RESULT
	case 0, DewfordTown_Hall_EventScript_TrendDebate1
	case 1, DewfordTown_Hall_EventScript_TrendDebate1
	case 2, DewfordTown_Hall_EventScript_TrendDebate2
	case 3, DewfordTown_Hall_EventScript_TrendDebate2
	case 4, DewfordTown_Hall_EventScript_TrendDebate3
	case 5, DewfordTown_Hall_EventScript_TrendDebate3
	case 6, DewfordTown_Hall_EventScript_TrendDebate4
	case 7, DewfordTown_Hall_EventScript_TrendDebate5
	end

DewfordTown_Hall_EventScript_TrendDebate1::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_SawTrendCoolestThing, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_AllegedTrendNotAuthentic, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate2::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_ComposedTrendThemeSong, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_WorkOnYourSinging, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate3::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_OrganizeTrendParty, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_BrilliantIndeed, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate4::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_TrendHasBecomePartOfLife, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_TrendWeighsHeavilyOnMind, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_TrendDebate5::
	call DewfordTown_Hall_EventScript_DebateReact1
	msgbox DewfordTown_Hall_Text_IfWeTeamUpWellBeInvincible, MSGBOX_DEFAULT
	call DewfordTown_Hall_EventScript_DebateReact2
	msgbox DewfordTown_Hall_Text_WellBeTrendDuo, MSGBOX_DEFAULT
	releaseall
	end

DewfordTown_Hall_EventScript_DontMovePlayer1::
	return

DewfordTown_Hall_EventScript_DebateReact1::
	applymovement LOCALID_PSYCHIC_M, DewfordTown_Hall_Movement_PsychicWalkInPlaceLeft
	waitmovement 0
	goto_if_eq VAR_0x8008, 0, DewfordTown_Hall_EventScript_PlayerReactWest
	goto_if_eq VAR_0x8008, 1, DewfordTown_Hall_EventScript_DontMovePlayer2
	end

DewfordTown_Hall_EventScript_PlayerReactWest::
	goto_if_eq VAR_FACING, DIR_EAST, DewfordTown_Hall_EventScript_DontMovePlayer1
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	return

DewfordTown_Hall_EventScript_DontMovePlayer2::
	return

DewfordTown_Hall_EventScript_DebateReact2::
	applymovement LOCALID_SCHOOL_KID_M, DewfordTown_Hall_Movement_SchoolKidWalkInPlaceRight
	waitmovement 0
	goto_if_eq VAR_0x8008, 0, DewfordTown_Hall_EventScript_PlayerReactNorthSouth
	goto_if_eq VAR_0x8008, 1, DewfordTown_Hall_EventScript_PlayerReactEast
	end

DewfordTown_Hall_EventScript_PlayerReactNorthSouth::
	call_if_eq VAR_FACING, DIR_NORTH, DewfordTown_Hall_EventScript_PlayerWalkInPlaceUp
	call_if_eq VAR_FACING, DIR_SOUTH, DewfordTown_Hall_EventScript_PlayerWalkInPlaceDown
	return

DewfordTown_Hall_EventScript_PlayerWalkInPlaceUp::
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	return

DewfordTown_Hall_EventScript_PlayerWalkInPlaceDown::
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	return

DewfordTown_Hall_EventScript_PlayerReactEast::
	goto_if_eq VAR_FACING, DIR_WEST, DewfordTown_Hall_EventScript_DontMovePlayer1
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	return

DewfordTown_Hall_Movement_PsychicWalkInPlaceLeft:
	walk_in_place_left
	step_end

DewfordTown_Hall_Movement_SchoolKidWalkInPlaceRight:
	walk_in_place_right
	step_end

DewfordTown_Hall_EventScript_SludgeBombMan::
	lock
	faceplayer
	call Common_EventScript_BufferTrendyPhrase
	goto_if_set FLAG_RECEIVED_TM36, DewfordTown_Hall_EventScript_ReceivedSludgeBomb
	msgbox DewfordTown_Hall_Text_GiveYouSludgeBomb, MSGBOX_DEFAULT
	giveitem ITEM_TM36
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_TM36
	release
	end

DewfordTown_Hall_EventScript_ReceivedSludgeBomb::
	msgbox DewfordTown_Hall_Text_LoveSludgeBombButTrendInToo, MSGBOX_DEFAULT
	release
	end

DewfordTown_Hall_Text_CantImagineLifeWithoutTrend:
	.string "En yeni moda şey ne mi?\n{STR_VAR_1} tabii ki!\p{STR_VAR_1} olmadan hayatın nasıl\nolacağını hayal bile edemiyorum!$"

DewfordTown_Hall_Text_GettingBoredOfTrend:
	.string "En yeni moda şey ne mi?\n{STR_VAR_1} tabii ki!\pAma bundan sıkılmaya başladım.\pBir sonraki büyük şeyi aramalıyım.$"

DewfordTown_Hall_Text_TeachingMonAboutTrend:
	.string "POKéMON'uma {STR_VAR_1} hakkında\nbir şeyler öğretiyorum ama pek\liyi gitmiyor.\pSanırım biraz fazla geldi.$"

DewfordTown_Hall_Text_DeepLinkBetweenXAndY:
	.string "Şu anda moda olan {STR_VAR_1}\nüzerinde çalışıyorum.\p{STR_VAR_1} ve {STR_VAR_2} arasında\nderin bir bağlantı olduğu doğru mu?$"

DewfordTown_Hall_Text_MyHunchWasRight:
	.string "Hey!\nDemek önsezim doğruymuş!\pHavalı ve popüler olmaya bir adım daha\nyaklaştım, yupi!$"

DewfordTown_Hall_Text_NotEasyToKeepUp:
	.string "Ne?! Öyle mi?!\pBenim gibi yaşlı bir adam için modayı\ntakip etmek kolay değil!$"

DewfordTown_Hall_Text_TVShowAboutTrend:
	.string "Tüm bu {STR_VAR_1}\nmeselesi…\pBununla ilgili bir televizyon\nprogramı yok mu?$"

DewfordTown_Hall_Text_IsTrendMorePopularAcrossSea:
	.string "Denizin ötesinde de…\n{STR_VAR_1} popüler midir?$"

DewfordTown_Hall_Text_CollectTrendMerchandise:
	.string "Resmi {STR_VAR_1}\nürün koleksiyonu yapıyorum.\pBende resmi {STR_VAR_1}var.\pResmi {STR_VAR_1} marka kıyafet…\pVe resmi lisanslı {STR_VAR_1} resimli\nkitapları.\pHeheh, ben sahibim! Sizinle hiçbir şey\npaylaşmıyorum!$"

DewfordTown_Hall_Text_BooksAboutTrend:
	.string "İYİ {STR_VAR_1} VE\nKÖTÜ {STR_VAR_1}…\pYARARLI {STR_VAR_1}…\p{STR_VAR_1} ile ilgili\nkitaplarla tıka basa dolu.$"

DewfordTown_Hall_Text_TrendsScream:
	.string "Başlıkta, {STR_VAR_1}'IN\nÇIĞLIĞI yazıyor.$"

DewfordTown_Hall_Text_TrendsSmile:
	.string "Başlıkta, {STR_VAR_1}'IN\nGÜLÜMSEMESİ yazıyor.$"

DewfordTown_Hall_Text_LastTrend:
	.string "Başlıkta, SON {STR_VAR_1}\nyazıyor.$"

DewfordTown_Hall_Text_BirthOfTrend:
	.string "Başlıkta, {STR_VAR_1}'IN\nDOĞUMU yazıyor.$"

DewfordTown_Hall_Text_SawTrendCoolestThing:
	.string "{STR_VAR_1} gördüm!\nHarika, değil mi?\pBu, olabilecek en havalı şey!\p$"

DewfordTown_Hall_Text_AllegedTrendNotAuthentic:
	.string "Ya, hayır, hayır, hayır.\pGördüğünü iddia ettiğiniz sözde\n{STR_VAR_1} bu değil.\pGerçek olan çok daha…\nNasıl desem?\lDaha keskin, ama daha yumuşak!$"

DewfordTown_Hall_Text_ComposedTrendThemeSong:
	.string "Hey, dinle, {STR_VAR_1}\niçin bir tema şarkısı yazdım.\p{STR_VAR_1} şu anda\nçok popüler.\pTamam, dinle!\n… …\pHey! {STR_VAR_1}!\pŞahane {STR_VAR_1}!$"

DewfordTown_Hall_Text_WorkOnYourSinging:
	.string "… …\pDiyebileceğim şu ki dostum,\n{STR_VAR_1} ile\luğraşmadan önce şarkı söylemeye\lçalışsan iyi olur.$"

DewfordTown_Hall_Text_OrganizeTrendParty:
	.string "Her neyse, daha önce de söylediğim\ngibi, bir araya gelmeli ve adada bir\l{STR_VAR_1} partisi\ldüzenlemeliyiz.$"

DewfordTown_Hall_Text_BrilliantIndeed:
	.string "Bu harika bir fikir!\p{STR_VAR_1} konusunda kimin en iyi\nolduğu ilk ve son kez ortaya çıkacak.\lGerçekten harika!\pBugünden itibaren hayatlarımız\n{STR_VAR_1} etrafında\ldönecek!$"

DewfordTown_Hall_Text_TrendHasBecomePartOfLife:
	.string "Düşünüyordum da…\pSence {STR_VAR_1}\nmodaya uygun bir şey olmaktan\lçıkıp günlük hayatımızın bir\lparçası haline gelmedi mi?$"

DewfordTown_Hall_Text_TrendWeighsHeavilyOnMind:
	.string "Pardon?\nO kadar mı, ne?\pAncak, ister uyanık ister uykuda olun,\n{STR_VAR_1}ın zihninizde büyük bir\lağırlık oluşturduğu doğrudur.\pKesinlikle, {STR_VAR_1} sizsiniz.\nProfesör, eski dostum!$"

DewfordTown_Hall_Text_IfWeTeamUpWellBeInvincible:
	.string "Eğer sen ve ben bir kombo olarak bir\naraya gelirsek, konu\l{STR_VAR_1} olduğunda\lyenilmez olacağız!$"

DewfordTown_Hall_Text_WellBeTrendDuo:
	.string "Tam isabet, dostum!\pBiz {STR_VAR_1}\nİKİLİSİ olacağız!\pBu çok iyi olmaz mıydı?\nHahahah!$"

DewfordTown_Hall_Text_GiveYouSludgeBomb:
	.string "Benim için BALÇIK BOMBA,\npopülerliğinin zirvesinde.\pHım?\nBana BALÇIK BOMBA'yı bilmediğini\lmi söylüyorsun?\pBu çok acınası.\nSana bir tane vereyim.$"

DewfordTown_Hall_Text_LoveSludgeBombButTrendInToo:
	.string "BALÇIK BOMBA'yı seviyorum.\pAma {STR_VAR_1}\nolan POKéMON da var.$"


