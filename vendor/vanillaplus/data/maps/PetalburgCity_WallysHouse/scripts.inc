.set LOCALID_WALLYS_DAD, 1

PetalburgCity_WallysHouse_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, PetalburgCity_WallysHouse_OnFrame
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, PetalburgCity_WallysHouse_OnWarp
	.byte 0

PetalburgCity_WallysHouse_OnWarp:
	map_script_2 VAR_PETALBURG_CITY_STATE, 4, PetalburgCity_WallysHouse_EventScript_PlayerWallysDadFaceEachOther
	.2byte 0

PetalburgCity_WallysHouse_EventScript_PlayerWallysDadFaceEachOther::
	turnobject OBJ_EVENT_ID_PLAYER, DIR_EAST
	turnobject LOCALID_WALLYS_DAD, DIR_WEST
	end

PetalburgCity_WallysHouse_OnFrame:
	map_script_2 VAR_PETALBURG_CITY_STATE, 4, PetalburgCity_WallysHouse_EventScript_GiveHM03Surf
	.2byte 0

PetalburgCity_WallysHouse_EventScript_GiveHM03Surf::
	lockall
	msgbox PetalburgCity_WallysHouse_Text_PleaseExcuseUs, MSGBOX_DEFAULT
	giveitem ITEM_HM03
	setflag FLAG_RECEIVED_HM03
	msgbox PetalburgCity_WallysHouse_Text_SurfGoAllSortsOfPlaces, MSGBOX_DEFAULT
	setvar VAR_PETALBURG_CITY_STATE, 5
	releaseall
	end

PetalburgCity_WallysHouse_EventScript_WallysDad::
	lock
	faceplayer
	goto_if_set FLAG_DEFEATED_WALLY_VICTORY_ROAD, PetalburgCity_WallysHouse_EventScript_DefeatedWallyInVictoryRoad
	goto_if_set FLAG_RECEIVED_HM03, PetalburgCity_WallysHouse_EventScript_ReceievedHM03Surf
	goto_if_set FLAG_THANKED_FOR_PLAYING_WITH_WALLY, PetalburgCity_WallysHouse_EventScript_PlayedWithWally
	msgbox PetalburgCity_WallysHouse_Text_ThanksForPlayingWithWally, MSGBOX_DEFAULT
	setflag FLAG_THANKED_FOR_PLAYING_WITH_WALLY
	release
	end

PetalburgCity_WallysHouse_EventScript_ReceievedHM03Surf::
	msgbox PetalburgCity_WallysHouse_Text_WallyIsComingHomeSoon, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_DefeatedWallyInVictoryRoad::
	msgbox PetalburgCity_WallysHouse_Text_YouMetWallyInEverGrandeCity, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_PlayedWithWally::
	msgbox PetalburgCity_WallysHouse_Text_WonderHowWallyIsDoing, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_WallysMom::
	lock
	faceplayer
	goto_if_set FLAG_RECEIVED_HM03, PetalburgCity_WallysHouse_EventScript_ReceivedHM03Surf
	msgbox PetalburgCity_WallysHouse_Text_WallyWasReallyHappy, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_EventScript_ReceivedHM03Surf::
	msgbox PetalburgCity_WallysHouse_Text_WallyLeftWithoutTelling, MSGBOX_DEFAULT
	release
	end

PetalburgCity_WallysHouse_Text_ThanksForPlayingWithWally:
	.string "Sen… Ah, sen {PLAYER}{KUN}\nolmalısın!\pBiraz önce WALLY ile oynadığın için\nteşekkür ederim.\pBebekliğinden beri zayıf ve hasta.\pOnu bir süreliğine VERDANTURF\nKASABASI'ndaki akrabalarımın yanına\lgönderdik.\pOradaki hava buradakinden çok daha\ntemiz.\pEfendim? WALLY nerede mi?\nWALLY gideli çok oldu.\pAcaba şu an nerelerdedir?$"

PetalburgCity_WallysHouse_Text_WonderHowWallyIsDoing:
	.string "Bizim WALLY ne yapıyor acaba?$"

PetalburgCity_WallysHouse_Text_PleaseExcuseUs:
	.string "{PLAYER}{KUN}! Seni buraya kadar\nsürüklediğimiz için bağışla.\pAma WALLY'imiz VERDANTURF'e\ngittiğinden beri çok sağlıklı oldu.\pHepsini sana borçluyuz!\pWALLY kasabadan ayrıldığında ona bir\nPOKéMON yakalamasında yardım etmiştin,\ldeğil mi?\pSanırım bu WALLY'i çok mutlu etti.\pAslında, sadece WALLY'i değil. Beni\nde mutlu etti. Onun babasıyım.\pSenin gibi harika bir arkadaş kazandığı\niçin çok mutlu.\pBu bir rüşvet falan değil ama bunu almanı\ngerçekten çok isterim.$"

PetalburgCity_WallysHouse_Text_SurfGoAllSortsOfPlaces:
	.string "Eğer POKéMON'un sörf yapabiliyorsa,\nher türlü yere gidebilirsin.$"

PetalburgCity_WallysHouse_Text_WallyIsComingHomeSoon:
	.string "WALLY yakında eve geliyor. Onu dört\ngözle bekliyorum.$"

PetalburgCity_WallysHouse_Text_YouMetWallyInEverGrandeCity:
	.string "Ha? WALLY'i EVER GRANDE\nŞEHRİ'nde mi gördün?\pAh, {PLAYER}{KUN}, saçmalama.\pSağlığına kavuşmuş olabilir ama öyle\nuzak bir yere tek başına gidemez.$"

PetalburgCity_WallysHouse_Text_WallyWasReallyHappy:
	.string "WALLY bize bir POKéMON yakaladığını\nsöylediğinde gerçekten çok mutluydu.\pOnu böyle gülümserken görmeyeli uzun\nzaman olmuştu.$"

PetalburgCity_WallysHouse_Text_WallyLeftWithoutTelling:
	.string "Bunu kocamdan sır olarak saklamanı\nistiyorum.\pAma bizim WALLY kimseye haber vermeden\nVERDANTURF KASABASI'nı terk etti.\pBiliyorsun, WALLY zayıftır ama\nşaşırtıcı derecede güçlü bir iradeye\lsahiptir.\pBir gün sağ salim geri döneceğinden\neminim!$"


