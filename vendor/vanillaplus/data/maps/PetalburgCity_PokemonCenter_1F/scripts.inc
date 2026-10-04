.set LOCALID_NURSE, 1

PetalburgCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PetalburgCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

PetalburgCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_PETALBURG_CITY
	call Common_EventScript_UpdateBrineyLocation
	end

PetalburgCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_FatMan::
	msgbox PetalburgCity_PokemonCenter_1F_Text_PCStorageSystem, MSGBOX_NPC
	end

PetalburgCity_PokemonCenter_1F_EventScript_Youngster::
	msgbox PetalburgCity_PokemonCenter_1F_Text_OranBerryRegainedHP, MSGBOX_NPC
	end

PetalburgCity_PokemonCenter_1F_EventScript_Woman::
	lock
	faceplayer
	msgbox PetalburgCity_PokemonCenter_1F_Text_ManyTypesOfPokemon, MSGBOX_DEFAULT
	specialvar VAR_RESULT, IsStarterInParty
	goto_if_eq VAR_RESULT, TRUE, PetalburgCity_PokemonCenter_1F_EventScript_SayStarterTypeInfo
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_SayStarterTypeInfo::
	call_if_eq VAR_STARTER_MON, 0, PetalburgCity_PokemonCenter_1F_EventScript_SayTreeckoType
	call_if_eq VAR_STARTER_MON, 1, PetalburgCity_PokemonCenter_1F_EventScript_SayTorchicType
	call_if_eq VAR_STARTER_MON, 2, PetalburgCity_PokemonCenter_1F_EventScript_SayMudkipType
	release
	end

PetalburgCity_PokemonCenter_1F_EventScript_SayTreeckoType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_TreeckoIsGrassType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_EventScript_SayTorchicType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_TorchicIsFireType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_EventScript_SayMudkipType::
	msgbox PetalburgCity_PokemonCenter_1F_Text_MudkipIsWaterType, MSGBOX_DEFAULT
	return

PetalburgCity_PokemonCenter_1F_Text_PCStorageSystem:
	.string "PC tabanlı POKéMON Depolama Sistemi…\pBunu her kim yaptıysa bir tür bilimsel\nsihirbaz olmalı!$"

PetalburgCity_PokemonCenter_1F_Text_OranBerryRegainedHP:
	.string "POKéMON'um bir ORAN MEYVESİ yediğinde,\ncanını geri kazandı!$"

PetalburgCity_PokemonCenter_1F_Text_ManyTypesOfPokemon:
	.string "Birçok POKéMON türü vardır.\pTüm türlerin birbirine karşı güçlü\nve zayıf yönleri vardır.\pPOKéMON türlerine bağlı olarak, bir\nsavaş kolay veya zor olabilir.$"

PetalburgCity_PokemonCenter_1F_Text_TreeckoIsGrassType:
	.string "Örneğin, sendeki TREECKO, bir\nÇİMEN türüdür.\pSU ve TOPRAK türlerine karşı güçlüdür.\pAncak, ATEŞ tipi POKéMON'lara\nkarşı zayıftır.$"

PetalburgCity_PokemonCenter_1F_Text_TorchicIsFireType:
	.string "Örneğin, sendeki TORCHIC, bir\nATEŞ türüdür.\pÇİMEN ve BÖCEK türlerine karşı\ngüçlüdür.\pAncak, SU tipi POKéMON'lara\nkarşı zayıftır.$"

PetalburgCity_PokemonCenter_1F_Text_MudkipIsWaterType:
	.string "Örneğin, MUDKIP'in bir SU türüdür.\pATEŞ türüne karşı güçlüdür.\pAncak, ÇİMEN tipi ve ELEKTRİK tipi\nPOKéMON'lara karşı zayıftır.$"


