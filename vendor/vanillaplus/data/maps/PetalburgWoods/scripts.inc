.set LOCALID_GRUNT, 3
.set LOCALID_DEVON_EMPLOYEE, 4

PetalburgWoods_MapScripts::
	.byte 0

PetalburgWoods_EventScript_DevonResearcherLeft::
	lockall
	call PetalburgWoods_EventScript_DevonResearcherIntro
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherApproachPlayerLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_HaveYouSeenShroomish, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaEntrance
	waitmovement 0
	msgbox PetalburgWoods_Text_IWasGoingToAmbushYou, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaApproachResearcherLeft
	waitmovement 0
	applymovement LOCALID_DEVON_EMPLOYEE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_HandOverThosePapers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherFleeToPlayerLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_YouHaveToHelpMe, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaApproachPlayer
	waitmovement 0
	msgbox PetalburgWoods_Text_NoOneCrossesTeamAqua, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_GRUNT_PETALBURG_WOODS, PetalburgWoods_Text_YoureKiddingMe
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaBackOff
	waitmovement 0
	call PetalburgWoods_EventScript_DevonResearcherPostBattle
	applymovement OBJ_EVENT_ID_PLAYER, PetalburgWoods_Movement_WatchResearcherLeave
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherExitLeft
	waitmovement 0
	goto PetalburgWoods_EventScript_RemoveDevonResearcher
	end

PetalburgWoods_EventScript_DevonResearcherRight::
	lockall
	call PetalburgWoods_EventScript_DevonResearcherIntro
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherApproachPlayerRight
	waitmovement 0
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterLeft
	waitmovement 0
	msgbox PetalburgWoods_Text_HaveYouSeenShroomish, MSGBOX_DEFAULT
	closemessage
	playbgm MUS_ENCOUNTER_AQUA, FALSE
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaEntrance
	waitmovement 0
	msgbox PetalburgWoods_Text_IWasGoingToAmbushYou, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaApproachResearcherRight
	waitmovement 0
	applymovement LOCALID_DEVON_EMPLOYEE, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_HandOverThosePapers, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherFleeToPlayerRight
	waitmovement 0
	msgbox PetalburgWoods_Text_YouHaveToHelpMe, MSGBOX_DEFAULT
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterUp
	waitmovement 0
	msgbox PetalburgWoods_Text_NoOneCrossesTeamAqua, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_GRUNT_PETALBURG_WOODS, PetalburgWoods_Text_YoureKiddingMe
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaBackOff
	waitmovement 0
	call PetalburgWoods_EventScript_DevonResearcherPostBattle
	applymovement OBJ_EVENT_ID_PLAYER, PetalburgWoods_Movement_WatchResearcherLeave
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherExitRight
	waitmovement 0
	goto PetalburgWoods_EventScript_RemoveDevonResearcher
	end

PetalburgWoods_EventScript_DevonResearcherIntro::
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherLookAround
	waitmovement 0
	msgbox PetalburgWoods_Text_NotAOneToBeFound, MSGBOX_DEFAULT
	closemessage
	return

PetalburgWoods_EventScript_DevonResearcherPostBattle::
	msgbox PetalburgWoods_Text_YouveGotSomeNerve, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_GRUNT, PetalburgWoods_Movement_AquaRunAway
	waitmovement 0
	removeobject LOCALID_GRUNT
	applymovement OBJ_EVENT_ID_PLAYER, Common_Movement_WalkInPlaceFasterDown
	waitmovement 0
	msgbox PetalburgWoods_Text_ThatWasAwfullyClose, MSGBOX_DEFAULT
	giveitem ITEM_GREAT_BALL
	goto_if_eq VAR_RESULT, FALSE, PetalburgWoods_EventScript_BagFull
	goto PetalburgWoods_EventScript_DevonResearcherFinish
	end

PetalburgWoods_EventScript_BagFull::
	msgbox PetalburgWoods_Text_YoureLoadedWithItems, MSGBOX_DEFAULT
	goto PetalburgWoods_EventScript_DevonResearcherFinish
	end

PetalburgWoods_EventScript_DevonResearcherFinish::
	msgbox PetalburgWoods_Text_TeamAquaAfterSomethingInRustboro, MSGBOX_DEFAULT
	applymovement LOCALID_DEVON_EMPLOYEE, PetalburgWoods_Movement_DevonResearcherStartExit
	waitmovement 0
	msgbox PetalburgWoods_Text_ICantBeWastingTime, MSGBOX_DEFAULT
	closemessage
	return

PetalburgWoods_EventScript_RemoveDevonResearcher::
	removeobject LOCALID_DEVON_EMPLOYEE
	setvar VAR_PETALBURG_WOODS_STATE, 1
	releaseall
	end

PetalburgWoods_Movement_DevonResearcherLookAround:
	face_up
	delay_16
	delay_4
	face_right
	delay_16
	delay_8
	face_left
	delay_16
	delay_8
	face_down
	delay_16
	face_right
	delay_16
	delay_8
	face_up
	delay_16
	step_end

PetalburgWoods_Movement_DevonResearcherExitLeft:
	walk_fast_right
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

PetalburgWoods_Movement_DevonResearcherApproachPlayerLeft:
	delay_16
	face_player
	walk_down
	walk_down
	step_end

PetalburgWoods_Movement_DevonResearcherApproachPlayerRight:
	delay_16
	face_player
	walk_down
	walk_down
	walk_down
	walk_in_place_faster_right
	step_end

PetalburgWoods_Movement_DevonResearcherExitRight:
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	step_end

PetalburgWoods_Movement_WatchResearcherLeave:
	delay_16
	delay_16
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherFleeToPlayerLeft:
	walk_fast_right
	walk_fast_down
	walk_fast_down
	walk_fast_left
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherFleeToPlayerRight:
	walk_fast_down
	walk_fast_right
	walk_in_place_faster_up
	step_end

PetalburgWoods_Movement_DevonResearcherStartExit:
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	face_up
	step_end

PetalburgWoods_Movement_AquaApproachResearcherLeft:
	walk_fast_down
	walk_fast_down
	step_end

PetalburgWoods_Movement_AquaBackOff:
	lock_facing_direction
	walk_up
	unlock_facing_direction
	step_end

PetalburgWoods_Movement_AquaRunAway:
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	delay_16
	delay_16
	step_end

PetalburgWoods_Movement_AquaApproachResearcherRight:
	walk_fast_down
	walk_fast_down
	walk_fast_down
	step_end

PetalburgWoods_Movement_AquaEntrance:
	walk_down
	walk_down
	delay_16
	delay_16
	step_end

PetalburgWoods_Movement_AquaApproachPlayer:
	walk_down
	step_end

PetalburgWoods_EventScript_Boy1::
	msgbox PetalburgWoods_Text_StayOutOfTallGrass, MSGBOX_NPC
	end

PetalburgWoods_EventScript_Boy2::
	msgbox PetalburgWoods_Text_HiddenItemsExplanation, MSGBOX_NPC
	end

PetalburgWoods_EventScript_Girl::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_MIRACLE_SEED, PetalburgWoods_EventScript_ExplainMiracleSeed
	msgbox PetalburgWoods_Text_TryUsingThisItem, MSGBOX_DEFAULT
	giveitem ITEM_MIRACLE_SEED
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_MIRACLE_SEED
	release
	end

PetalburgWoods_EventScript_ExplainMiracleSeed::
	msgbox PetalburgWoods_Text_MiracleSeedExplanation, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_Sign1::
	msgbox PetalburgWoods_Text_TrainerTipsExperience, MSGBOX_SIGN
	end

PetalburgWoods_EventScript_Sign2::
	msgbox PetalburgWoods_Text_TrainerTipsPP, MSGBOX_SIGN
	end

PetalburgWoods_EventScript_Lyle::
	trainerbattle_single TRAINER_LYLE, PetalburgWoods_Text_GoBugPokemonTeam, PetalburgWoods_Text_ICouldntWin
	msgbox PetalburgWoods_Text_ImOutOfPokeBalls, MSGBOX_AUTOCLOSE
	end

PetalburgWoods_EventScript_James::
	trainerbattle_single TRAINER_JAMES_1, PetalburgWoods_Text_InstantlyPopularWithBugPokemon, PetalburgWoods_Text_CantBePopularIfILose, PetalburgWoods_EventScript_TryRegisterJames
	specialvar VAR_RESULT, ShouldTryRematchBattle
	goto_if_eq VAR_RESULT, TRUE, PetalburgWoods_EventScript_JamesRematch
	setvar VAR_0x8004, TRAINER_JAMES_1
	specialvar VAR_RESULT, IsTrainerRegistered
	goto_if_eq VAR_RESULT, FALSE, PetalburgWoods_EventScript_TryRegisterJames2
	msgbox PetalburgWoods_Text_PeopleRespectYou, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_TryRegisterJames::
	special PlayerFaceTrainerAfterBattle
	waitmovement 0
	goto_if_set FLAG_HAS_MATCH_CALL, PetalburgWoods_EventScript_RegisterJames
	release
	end

PetalburgWoods_EventScript_RegisterJames::
	msgbox PetalburgWoods_Text_IWantRematch1, MSGBOX_DEFAULT
	register_matchcall TRAINER_JAMES_1
	release
	end

PetalburgWoods_EventScript_TryRegisterJames2::
	goto_if_set FLAG_HAS_MATCH_CALL, PetalburgWoods_EventScript_RegisterJames2
	msgbox PetalburgWoods_Text_PeopleRespectYou, MSGBOX_DEFAULT
	release
	end

PetalburgWoods_EventScript_RegisterJames2::
	msgbox PetalburgWoods_Text_IWantRematch2, MSGBOX_DEFAULT
	register_matchcall TRAINER_JAMES_1
	release
	end

PetalburgWoods_EventScript_JamesRematch::
	trainerbattle_rematch TRAINER_JAMES_1, PetalburgWoods_Text_MyPokemonHaveGrown, PetalburgWoods_Text_CantBePopularIfLose
	msgbox PetalburgWoods_Text_IveBeenTrainingHard, MSGBOX_AUTOCLOSE
	end

PetalburgWoods_Text_NotAOneToBeFound:
	.string "Hmmm…\nBir tane bile bulamadım…$"

PetalburgWoods_Text_HaveYouSeenShroomish:
	.string "Merhaba, buralarda SHROOMISH adında bir\nPOKéMON gördün mü?\pO POKéMON'u çok severim.$"

PetalburgWoods_Text_IWasGoingToAmbushYou:
	.string "Seni pusuya düşürecektim ama PETALBURG\nORMANI'nda takılı kaldın, değil mi?\pBeklemekten sıkıldım ve işte buradayım!$"

PetalburgWoods_Text_HandOverThosePapers:
	.string "Sen! DEVON ARAŞTIRMACISI!\pVer şu kağıtları!$"

PetalburgWoods_Text_YouHaveToHelpMe:
	.string "İyeeeh!\pSen bir POKéMON eğitmenisin, değil mi?\nBana yardım etmelisin, lütfen!$"

PetalburgWoods_Text_NoOneCrossesTeamAqua:
	.string "Hah? Ne yaptığını sanıyorsun sen? Ne,\nonu koruyacak mısın?\pAQUA TAKIMI'na karşı gelen hiç kimse\nmerhamet görmez, bir çocuk bile!\pGel de dövüş benimle!$"

PetalburgWoods_Text_YoureKiddingMe:
	.string "Şaka yapıyor olmalısın!\nÇok sertsin!$"

PetalburgWoods_Text_YouveGotSomeNerve:
	.string "Grrr… AQUA TAKIMI'na burnunu sokmaya\ncüret ediyorsun! Hadi gel ve benimle\ltekrar savaş!\pKeşke bunu diyebilseydim, ama\nPOKéMON'um bitti…\pVe, hey, AQUA TAKIMI olarak biz de\nRUSTBORO'da bir şeylerin peşindeyiz.\pKaçmana bugünlük izin vereceğim!$"

PetalburgWoods_Text_ThatWasAwfullyClose:
	.string "Vay canına… Çok yakındı!\pSayende, bu önemli belgeleri benden\nçalamadı.\pBiliyorum, teşekkür olarak sana\nbunu vereceğim!$"

PetalburgWoods_Text_TeamAquaAfterSomethingInRustboro:
	.string "O AQUA TAKIMI haydutu RUSTBORO'da\nşeyin peşinde olduklarını mı\lsöyledi?$"

PetalburgWoods_Text_ICantBeWastingTime:
	.string "Olamaz! Bu çok kötü!\nZaman kaybedemem!$"

PetalburgWoods_Text_YoureLoadedWithItems:
	.string "Çantan dolu.\nSana bu topu veremiyorum.$"

PetalburgWoods_Text_GoBugPokemonTeam:
	.string "Bir sürü POKéMON yakaladım!\pHadi, hadi, hadi!\nBÖCEK POKéMON'u takımım!$"

PetalburgWoods_Text_ICouldntWin:
	.string "Bütün bu POKéMON'lara sahibim, ama\nkazanamadım…$"

PetalburgWoods_Text_ImOutOfPokeBalls:
	.string "Bir sürü POKéMON yakaladım.\nAma POKé TOPLARIM bitti.$"

PetalburgWoods_Text_InstantlyPopularWithBugPokemon:
	.string "BÖCEK POKéMON'larını okula götürürsen\nanında popüler olursun!$"

PetalburgWoods_Text_CantBePopularIfILose:
	.string "Kaybedersem popüler olamam.$"

PetalburgWoods_Text_PeopleRespectYou:
	.string "Eğer büyük bir BÖCEK POKéMON'un varsa\ninsanlar bunun için size saygı duyar.$"

PetalburgWoods_Text_IWantRematch1:
	.string "BÖCEK POKéMON'um büyüdüğünde rövanş\nistiyorum!\pSeni POKéNAV'ıma kaydediyorum!$"

PetalburgWoods_Text_IWantRematch2:
	.string "BÖCEK POKéMON'um büyüdüğünde rövanş\nistiyorum!\pSeni POKéNAV'ıma kaydediyorum!$"

PetalburgWoods_Text_MyPokemonHaveGrown:
	.string "BÖCEK POKéMON'um büyüdü.\nTekrar popüler olacağım.$"

PetalburgWoods_Text_CantBePopularIfLose:
	.string "Kaybedersem popüler olamam.$"

PetalburgWoods_Text_IveBeenTrainingHard:
	.string "Güçlü bir POKéMON'un varsa popüler\nolursun, değil mi? Bu yüzden sıkı\lçalışıyorum.$"

PetalburgWoods_Text_StayOutOfTallGrass:
	.string "Hey, sen! POKéMON'un iyi mi?\pEğer POKéMON'un zayıfsa ve savaşlardan\nkaçınmak istiyorsan, uzun otlardan uzak\ldurmalısın.$"

PetalburgWoods_Text_HiddenItemsExplanation:
	.string "Bazen göremeseniz bile yerde bir şeyler\nolabilir.\pBu yüzden her zaman yürüdüğüm yeri\nkontrol ederim.$"

PetalburgWoods_Text_TryUsingThisItem:
	.string "Ah, harika\nBu RUSTBORO SALON rozeti!\pSen bir EĞİTMEN olmalısın.\nBu eşyayı kullanmayı dene.$"

PetalburgWoods_Text_MiracleSeedExplanation:
	.string "Bu bir MUCİZE TOHUM. Eğer bir POKéMON\nbu eşyayı tutarsa, ÇİMEN saldırıları\ldaha güçlü hale gelir.\pMUCİZE TOHUM dışında, POKéMON'un\ntutması için başka uygun eşyalar da\lolduğunu düşünüyorum.$"

PetalburgWoods_Text_TrainerTipsExperience:
	.string "EĞİTMEN İPUÇLARI\pBir savaşta bir kez bile görünen her\nPOKéMON EXP Puanı kazanır.\pZayıf bir POKéMON'u yükseltmek için, onu\ntakım listesinin soluna koyun.\pBir savaş başlar başlamaz onu\ndeğiştirin. Herhangi bir zarara maruz\lkalmadan EXP Puanı kazanacaktır.$"

PetalburgWoods_Text_TrainerTipsPP:
	.string "EĞİTMEN İPUÇLARI\pCana ek olarak, POKéMON'un savaş\nsırasında saldırı yapmak için\lkullandığı Güç Puanları (PP)\lvardır.\pEğer bir POKéMON'un PP'si biterse, bir\nPOKéMON MERKEZİ'ne götürülmelidir.$"


