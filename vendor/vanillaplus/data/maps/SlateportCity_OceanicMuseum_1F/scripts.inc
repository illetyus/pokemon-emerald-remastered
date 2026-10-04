.set LOCALID_FAMILIAR_GRUNT, 13

SlateportCity_OceanicMuseum_1F_MapScripts::
	.byte 0

SlateportCity_OceanicMuseum_1F_EventScript_EntranceAttendant::
	msgbox SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFeeLeft::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFeeRight::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterRight
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee
	end

SlateportCity_OceanicMuseum_1F_EventScript_PayEntranceFee::
	showmoneybox 0, 0
	msgbox SlateportCity_OceanicMuseum_1F_Text_WouldYouLikeToEnter, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, SlateportCity_OceanicMuseum_1F_EventScript_CheckMoneyForFee
	closemessage
	hidemoneybox
	applymovement OBJ_EVENT_ID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_CheckMoneyForFee::
	checkmoney 50
	goto_if_eq VAR_RESULT, FALSE, SlateportCity_OceanicMuseum_1F_EventScript_NotEnoughMoney
	playse SE_SHOP
	removemoney 50
	updatemoneybox
	msgbox SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself, MSGBOX_DEFAULT
	setvar VAR_SLATEPORT_MUSEUM_1F_STATE, 1
	hidemoneybox
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_NotEnoughMoney::
	goto_if_unset FLAG_DELIVERED_DEVON_GOODS, SlateportCity_OceanicMuseum_1F_EventScript_AllowEntranceAnyway
	msgbox SlateportCity_OceanicMuseum_1F_Text_NotEnoughMoney, MSGBOX_DEFAULT
	closemessage
	hidemoneybox
	applymovement OBJ_EVENT_ID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter
	waitmovement 0
	releaseall
	end

SlateportCity_OceanicMuseum_1F_EventScript_AllowEntranceAnyway::
	msgbox SlateportCity_OceanicMuseum_1F_Text_CatchUpWithYourGroup, MSGBOX_DEFAULT
	setvar VAR_SLATEPORT_MUSEUM_1F_STATE, 1
	hidemoneybox
	releaseall
	end

SlateportCity_OceanicMuseum_1F_Movement_PushPlayerBackFromCounter:
	walk_down
	step_end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_AquaExistForGoodOfAll, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OurBossIsntHere, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WouldStuffHereMakeMeRich, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt4::
	msgbox SlateportCity_OceanicMuseum_1F_Text_CanLearnForNefariousDeeds, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt5::
	msgbox SlateportCity_OceanicMuseum_1F_Text_RustboroBungled, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumGrunt6::
	msgbox SlateportCity_OceanicMuseum_1F_Text_DidntHaveMoney, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_WhirlpoolExperiment::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WhirlpoolExperiment, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_WaterfallExperiment::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WaterfallExperiment, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanSoilDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanSoilDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_BeachSandDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_BeachSandDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact1, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact2, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_OceanicMinifact3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact3, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_FossilDisplay::
	msgbox SlateportCity_OceanicMuseum_1F_Text_FossilDisplay, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_DepthMeasuringMachine::
	msgbox SlateportCity_OceanicMuseum_1F_Text_DepthMeasuringMachine, MSGBOX_SIGN
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron1::
	msgbox SlateportCity_OceanicMuseum_1F_Text_LearnAboutSeaForBattling, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron2::
	msgbox SlateportCity_OceanicMuseum_1F_Text_SternIsRoleModel, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron3::
	msgbox SlateportCity_OceanicMuseum_1F_Text_MustBePokemonWeDontKnow, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_MuseumPatron4::
	msgbox SlateportCity_OceanicMuseum_1F_Text_WantSeaPokemon, MSGBOX_NPC
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGrunt::
	lock
	faceplayer
	delay 8
	playse SE_PIN
	applymovement LOCALID_FAMILIAR_GRUNT, Common_Movement_ExclamationMark
	waitmovement 0
	applymovement LOCALID_FAMILIAR_GRUNT, Common_Movement_Delay48
	waitmovement 0
	msgbox SlateportCity_OceanicMuseum_1F_Text_RememberMeTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM46
	goto_if_eq VAR_RESULT, 0, SlateportCity_OceanicMuseum_1F_EventScript_NoRoomForThief
	setflag FLAG_RECEIVED_TM46
	msgbox SlateportCity_OceanicMuseum_1F_Text_HopeINeverSeeYouAgain, MSGBOX_DEFAULT
	closemessage
	goto_if_eq VAR_FACING, DIR_NORTH, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitNorth
	goto_if_eq VAR_FACING, DIR_SOUTH, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitSouth
	goto_if_eq VAR_FACING, DIR_WEST, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast
	goto_if_eq VAR_FACING, DIR_EAST, SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitNorth::
	applymovement OBJ_EVENT_ID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitNorth
	applymovement LOCALID_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExitNorth
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitSouth::
	applymovement LOCALID_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExitWestEast::
	applymovement OBJ_EVENT_ID_PLAYER, SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitWestEast
	applymovement LOCALID_FAMILIAR_GRUNT, SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit
	waitmovement 0
	goto SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited
	end

SlateportCity_OceanicMuseum_1F_EventScript_FamiliarGruntExited::
	setflag FLAG_HIDE_SLATEPORT_CITY_OCEANIC_MUSEUM_FAMILIAR_AQUA_GRUNT
	playse SE_EXIT
	removeobject LOCALID_FAMILIAR_GRUNT
	release
	end

SlateportCity_OceanicMuseum_1F_EventScript_NoRoomForThief::
	msgbox SlateportCity_OceanicMuseum_1F_Text_YouHaveToTakeThis, MSGBOX_DEFAULT
	release
	end

SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitNorth:
	delay_16
	delay_8
	delay_4
	walk_in_place_faster_down
	step_end

SlateportCity_OceanicMuseum_1F_Movement_PlayerWatchGruntExitWestEast:
	delay_16
	walk_in_place_faster_down
	step_end

SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExit:
	face_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	delay_8
	step_end

SlateportCity_OceanicMuseum_1F_Movement_FamiliarGruntExitNorth:
	walk_fast_right
	walk_fast_down
	walk_fast_down
	walk_fast_down
	walk_fast_down
	delay_8
	step_end

SlateportCity_OceanicMuseum_1F_Text_WouldYouLikeToEnter:
	.string "OKYANUS MÜZESİ'ne hoş geldiniz.\pGiriş ücreti 50¥.\nGirmek ister misiniz?$"

SlateportCity_OceanicMuseum_1F_Text_PleaseEnjoyYourself:
	.string "Lütfen keyfinize bakın.$"

SlateportCity_OceanicMuseum_1F_Text_NotEnoughMoney:
	.string "Ha? Üzgünüm, ama yeterince paranız\nyok gibi görünüyor.$"

SlateportCity_OceanicMuseum_1F_Text_CatchUpWithYourGroup:
	.string "Ha?\nDaha önce içeri giren grupla mısın?\pGeç kalan bir tek sen varsın.\nGidip onlara yetişsen iyi olur!$"

SlateportCity_OceanicMuseum_1F_Text_AquaExistForGoodOfAll:
	.string "Biz AQUA TAKIMI, herkesin iyiliği\niçin varız!$"

SlateportCity_OceanicMuseum_1F_Text_OurBossIsntHere:
	.string "Burada toplanmamız söylendi, biz de\ntoplandık ama…\pPatronumuz burada değil.$"

SlateportCity_OceanicMuseum_1F_Text_WouldStuffHereMakeMeRich:
	.string "Eğer buradakileri çalsaydım, bu beni\nzengin eder miydi?$"

SlateportCity_OceanicMuseum_1F_Text_CanLearnForNefariousDeeds:
	.string "Burada öğrendiklerimi hain işlerde\nkullanabilirim.$"

SlateportCity_OceanicMuseum_1F_Text_RustboroBungled:
	.string "Eğer haydutumuz RUSTBORO'da işleri\nberbat etmeseydi, burada olmazdık!$"

SlateportCity_OceanicMuseum_1F_Text_DidntHaveMoney:
	.string "50¥'im yoktu, bu yüzden resepsiyon\ngörevlisini geçmem uzun zaman aldı.$"

SlateportCity_OceanicMuseum_1F_Text_LearnAboutSeaForBattling:
	.string "Deniz hakkında bilgi edinmek ve bu\nbilgiyi savaşmak için kullanmak\listiyorum.$"

SlateportCity_OceanicMuseum_1F_Text_SternIsRoleModel:
	.string "Denizi gördüğümde başım dönüyor ve\nyapış yapış oluyorum!\pBu yüzden KAPTAN STERN idolümdür!$"

SlateportCity_OceanicMuseum_1F_Text_MustBePokemonWeDontKnow:
	.string "Deniz, uçsuz bucaksız ve sonsuz\nderinlikte.\pBilmediğimiz bir sürü POKéMON olmalı.$"

SlateportCity_OceanicMuseum_1F_Text_WantSeaPokemon:
	.string "Bir deniz POKéMON'u istiyorum.\pBence olması çok havalı.\nAyrıca ona sarılmak güzel olurdu.$"

SlateportCity_OceanicMuseum_1F_Text_RememberMeTakeThis:
	.string "Aiyeeeh! Burada ne işin var senin?\pBen mi?\nBen daha önce patakladığın AQUA\lTAKIMI üyesiyim, hatırlamadın mı?\lRUSTURF TÜNELİ'nde hani.\pHey, al bunu!\nBeni affetmelisin!$"

SlateportCity_OceanicMuseum_1F_Text_HopeINeverSeeYouAgain:
	.string "Bu TM, sana benden daha çok yakışıyor.\pUmarım seni bir daha görmem!\nHahahaha!$"

SlateportCity_OceanicMuseum_1F_Text_YouHaveToTakeThis:
	.string "Aaa, hadi ama!\nBunu alıp peşimi bırakmalısın!$"

SlateportCity_OceanicMuseum_1F_Text_WhirlpoolExperiment:
	.string "Mavi bir sıvı cam bir kabın içinde spiral\nçiziyor.\pBu, rüzgarı kullanarak yapay bir\nHORTUM yaratmaya yönelik bir deney.$"

SlateportCity_OceanicMuseum_1F_Text_WaterfallExperiment:
	.string "Kırmızı bir top cam bir kabın içinde\naşağı yukarı sallanıyor.\pBu, topun kaldırma kuvvetini\nkullanarak bir ŞELALE simüle eden\lbir deneydir.$"

SlateportCity_OceanicMuseum_1F_Text_OceanSoilDisplay:
	.string "Okyanus tabanından alınan bir toprak\nörneği.\pUzun yıllar boyunca, yaşam formlarının\nkalıntıları denizin dibine çökerek\ltortul katmanlar oluşturur.\pBu katmanlar analiz edildiğinde, antik\ngeçmiş ortaya çıkar.$"

SlateportCity_OceanicMuseum_1F_Text_BeachSandDisplay:
	.string "Bu bir plaj kumu örneği.\pDağlardan gelen taşlar nehirler\ntarafından yıkanır ve burada\lyontularak ufalanır.\pTaneciklere dönüşürler ve plajlarda\nkum olarak son bulurlar.$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact1:
	.string "OKYANUS BİLGİLERİ 1\nDeniz suyu neden mavidir?\pIşık birçok renkten oluşur.\pIşık sudan geçtiğinde, çoğu ışık türü\nrengini kaybeder.\pAncak mavi ışık rengini korur ve\ndenizin mavi görünmesine neden olur.$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact2:
	.string "OKYANUS BİLGİLERİ 2\nDeniz neden tuzludur?\pDeniz suyu sodyum ve klor iyonları\nşeklinde çözünmüş tuz içerir.\pBu iyonlar kayalardan sızar ve\nyağmurla denize taşınır.\pÇözünmüş tuz konsantrasyonu denizi\ntuzlu yapar.$"

SlateportCity_OceanicMuseum_1F_Text_OceanicMinifact3:
	.string "OKYANUS BİLGİLERİ 3\nHangisi daha büyüktür?\lDeniz mi kara mı?\pDeniz gezegenin yaklaşık %70'ini\nkaplar, geri kalanı ise karadır.\pBu nedenle deniz, karanın iki katından\ndaha büyüktür.$"

SlateportCity_OceanicMuseum_1F_Text_FossilDisplay:
	.string "Üzerinde çıkıntılar olan bir fosil.\pOkyanus tabanındaki toprak gelgit\ntarafından aşındırılır.\pAkan deniz suyu toprağı küçük\nçıkıntılar ve vadilerle işaretler.\pEğer bu toprak fosilleşirse, buna\ndalgalanma izi denir.$"

SlateportCity_OceanicMuseum_1F_Text_DepthMeasuringMachine:
	.string "Cam bir kubbenin altında garip bir\nmakine dönüyor.\pBelki de bir şeyin derinliğini ölçmek\niçindir…$"


