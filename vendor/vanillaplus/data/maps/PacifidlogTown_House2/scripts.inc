PacifidlogTown_House2_MapScripts::
	.byte 0

PacifidlogTown_House2_EventScript_FanClubYoungerBrother::
	lock
	faceplayer
	dotimebasedevents
	call PacifidlogTown_House2_EventScript_UpdateFanClubTMFlag
	goto_if_set FLAG_RECEIVED_FANCLUB_TM_THIS_WEEK, PacifidlogTown_House2_EventScript_ComeBackInXDays
	call_if_set FLAG_MET_FANCLUB_YOUNGER_BROTHER, PacifidlogTown_House2_EventScript_MonAssessment
	call_if_unset FLAG_MET_FANCLUB_YOUNGER_BROTHER, PacifidlogTown_House2_EventScript_FirstMonAssessment
	setflag FLAG_MET_FANCLUB_YOUNGER_BROTHER
	specialvar VAR_RESULT, GetLeadMonFriendshipScore
	goto_if_ge VAR_RESULT, FRIENDSHIP_150_TO_199, PacifidlogTown_House2_EventScript_GiveReturn
	specialvar VAR_RESULT, GetLeadMonFriendshipScore
	goto_if_ge VAR_RESULT, FRIENDSHIP_50_TO_99, PacifidlogTown_House2_EventScript_PutInEffort
	goto PacifidlogTown_House2_EventScript_GiveFrustration
	end

PacifidlogTown_House2_EventScript_UpdateFanClubTMFlag::
	goto_if_unset FLAG_RECEIVED_FANCLUB_TM_THIS_WEEK, Common_EventScript_NopReturn
	specialvar VAR_RESULT, GetDaysUntilPacifidlogTMAvailable
	call_if_eq VAR_RESULT, 0, PacifidlogTown_House2_EventScript_ClearReceivedFanClubTM
	return

PacifidlogTown_House2_EventScript_MonAssessment::
	msgbox PacifidlogTown_House2_Text_AhYourPokemon, MSGBOX_DEFAULT
	return

PacifidlogTown_House2_EventScript_FirstMonAssessment::
	msgbox PacifidlogTown_House2_Text_ChairmansYoungerBrotherOnVacation, MSGBOX_DEFAULT
	msgbox PacifidlogTown_House2_Text_AhYourPokemon, MSGBOX_DEFAULT
	return

PacifidlogTown_House2_EventScript_ClearReceivedFanClubTM::
	clearflag FLAG_RECEIVED_FANCLUB_TM_THIS_WEEK
	return

PacifidlogTown_House2_EventScript_GiveReturn::
	msgbox PacifidlogTown_House2_Text_AdoringPokemonTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM27
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_FANCLUB_TM_THIS_WEEK
	special SetPacifidlogTMReceivedDay
	msgbox PacifidlogTown_House2_Text_ExplainReturnFrustration, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_EventScript_PutInEffort::
	msgbox PacifidlogTown_House2_Text_PutInSomeMoreEffort, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_EventScript_GiveFrustration::
	msgbox PacifidlogTown_House2_Text_ViciousPokemonTakeThis, MSGBOX_DEFAULT
	giveitem ITEM_TM21
	goto_if_eq VAR_RESULT, FALSE, Common_EventScript_ShowBagIsFull
	setflag FLAG_RECEIVED_FANCLUB_TM_THIS_WEEK
	special SetPacifidlogTMReceivedDay
	msgbox PacifidlogTown_House2_Text_ExplainReturnFrustration, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_EventScript_ComeBackInXDays::
	specialvar VAR_RESULT, GetDaysUntilPacifidlogTMAvailable
	buffernumberstring STR_VAR_1, VAR_RESULT
	msgbox PacifidlogTown_House2_Text_GetGoodTMInXDays, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_EventScript_HappyAzurill::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZURILL, CRY_MODE_NORMAL
	msgbox PacifidlogTown_House2_Text_Rurii, MSGBOX_DEFAULT
	waitmoncry
	msgbox PacifidlogTown_House2_Text_VeryFriendlyWithTrainer, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_EventScript_UnhappyAzurill::
	lock
	faceplayer
	waitse
	playmoncry SPECIES_AZURILL, CRY_MODE_ENCOUNTER
	msgbox PacifidlogTown_House2_Text_Rururi, MSGBOX_DEFAULT
	waitmoncry
	msgbox PacifidlogTown_House2_Text_DoesntLikeTrainerVeryMuch, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House2_Text_ChairmansYoungerBrotherOnVacation:
	.string "Er-hem!\pBen POKéMON FAN KULÜBÜ'nün en önemli\nkişisi, BAŞKAN'ın küçük kardeşiyim.\pBurada POKéMON'lar ile tatilimin\ntadını çıkarıyorum, evet, gerçekten.$"

PacifidlogTown_House2_Text_AhYourPokemon:
	.string "Ah!\nSenin POKéMON'un…$"

PacifidlogTown_House2_Text_AdoringPokemonTakeThis:
	.string "Belli ki seni çok seviyor.\pBu kadar sevgi dolu ve sevimli bir\nPOKéMON böyle bir TM'yi hak ediyor,\ldeğil mi?$"

PacifidlogTown_House2_Text_PutInSomeMoreEffort:
	.string "Hmm… Kötü değil, ama iyi de değil.\pEğitmen olarak biraz daha çaba\ngöstermen gerekiyor.$"

PacifidlogTown_House2_Text_ViciousPokemonTakeThis:
	.string "Vahşi bir görünümü var.\pBöyle korkutucu bir POKéMON böyle bir\nTM'yi hak ediyor.$"

PacifidlogTown_House2_Text_ExplainReturnFrustration:
	.string "Eğer bir POKéMON senden çok\nhoşlanıyorsa, KARŞILIK'ın gücü artar.\pEğer senden hoşlanmıyorsa,\nHÜSRAN'ın gücü artar.$"

PacifidlogTown_House2_Text_GetGoodTMInXDays:
	.string "Aa, evet. Yaklaşık {STR_VAR_1} gün içinde\niyi bir ya da iki TM alacağım.\pO zaman beni görmeye gelmelisin.\nSana POKéMON'una uygun bir TM veririm.$"

PacifidlogTown_House2_Text_Rurii:
	.string "AZURILL: Rurii.$"

PacifidlogTown_House2_Text_VeryFriendlyWithTrainer:
	.string "Eğitmeni ile çok dostane görünüyor.$"

PacifidlogTown_House2_Text_Rururi:
	.string "AZURILL: Rururi!$"

PacifidlogTown_House2_Text_DoesntLikeTrainerVeryMuch:
	.string "Eğitmeninden pek hoşlanmıyor gibi\ngörünüyor.$"


