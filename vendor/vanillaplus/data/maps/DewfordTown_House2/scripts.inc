DewfordTown_House2_MapScripts::
	.byte 0

DewfordTown_House2_EventScript_Man::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_SILK_SCARF, DewfordTown_House2_EventScript_ExplainSilkScarf
	msgbox DewfordTown_House2_Text_WantYouToHaveSilkScarf, MSGBOX_DEFAULT
	giveitem ITEM_SILK_SCARF
	goto_if_eq VAR_RESULT, FALSE, DewfordTown_House2_EventScript_NoRoomForScarf
	setflag FLAG_RECEIVED_SILK_SCARF
	release
	end

DewfordTown_House2_EventScript_NoRoomForScarf::
	msgbox DewfordTown_House2_Text_NoRoom, MSGBOX_DEFAULT
	release
	end

DewfordTown_House2_EventScript_ExplainSilkScarf::
	msgbox DewfordTown_House2_Text_ExplainSilkScarf, MSGBOX_DEFAULT
	release
	end

DewfordTown_House2_EventScript_Boy::
	msgbox DewfordTown_House2_Text_BrawlySoCool, MSGBOX_NPC
	end

DewfordTown_House2_Text_WantYouToHaveSilkScarf:
	.string "Çabuk buna bir bak!\pBu bir İPEK ATKI. Son moda\nbir aksesuar, evet!\pGözlerinin parladığını\ngörebiliyorum! Göz kamaştırıcı tarzımı\ltakdir ediyorsun!\pZevkin çok iyiymiş! Al bakalım.\nBunu almanı istiyorum!$"

DewfordTown_House2_Text_NoRoom:
	.string "Yerin yok mu?\pBeni iyi dinle, bu atkıyı mutlaka\nalmalısın! Ben olsam alabilmek için\ltüm eşyalarımı satardım!$"

DewfordTown_House2_Text_ExplainSilkScarf:
	.string "İPEK ATKI, NORMAL tip saldırı\ngücünü artırır.\pNeredeyse tüm POKéMON'larla uyum\nsağlayacak muhteşem bir ATKI'dır!$"

DewfordTown_House2_Text_BrawlySoCool:
	.string "Vay canına, DEWFORD'u ziyaret etmek\niçin denizi geçme zahmetine mi\lkatlandın?\pBuraya BRAWLY'i duyduğun için mi\ngeldin?\pO çok havalı... Herkes onu idol olarak\ngörüyor.$"


