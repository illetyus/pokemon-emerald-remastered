PacifidlogTown_House4_MapScripts::
	.byte 0

PacifidlogTown_House4_EventScript_LittleGirl::
	msgbox PacifidlogTown_House4_Text_SkyPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Woman::
	msgbox PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon, MSGBOX_NPC
	end

PacifidlogTown_House4_EventScript_Boy::
	lock
	faceplayer
	msgbox PacifidlogTown_House4_Text_WhereDidYouComeFrom, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, PacifidlogTown_House4_EventScript_Yes
	goto_if_eq VAR_RESULT, NO, PacifidlogTown_House4_EventScript_No
	end

PacifidlogTown_House4_EventScript_Yes::
	msgbox PacifidlogTown_House4_Text_YesTown, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_EventScript_No::
	msgbox PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere, MSGBOX_DEFAULT
	release
	end

PacifidlogTown_House4_Text_PeopleSawHighFlyingPokemon:
	.string "İnsanlar HOENN'in üzerinde uçan bir\nPOKéMON gördüklerini söylüyorlardı.\pSürekli etrafta mı uçuyor? Bir yerde\ndinlenmesi gerekmiyor mu?$"

PacifidlogTown_House4_Text_SkyPokemon:
	.string "Gökyüzü POKéMON'u!\nGökyüzü POKéMON'u!$"

PacifidlogTown_House4_Text_WhereDidYouComeFrom:
	.string "Sen nereden geldin?$"

PacifidlogTown_House4_Text_YesTown:
	.string "Evet mi?\nEVET KASABASI MI?\pBöyle bir yeri hiç duymamıştım.$"

PacifidlogTown_House4_Text_YouHaveToComeFromSomewhere:
	.string "Hayır mı? Bu hiç mantıklı değil.\nBir yerden gelmiş olmalısın.\pBekle! Denizin dibinde doğduğunu\nsöylemeyeceksin değil mi?$"


