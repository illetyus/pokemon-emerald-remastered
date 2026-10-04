.set LOCALID_NURSE, 1

OldaleTown_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, OldaleTown_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

OldaleTown_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_OLDALE_TOWN
	call Common_EventScript_UpdateBrineyLocation
	end

OldaleTown_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

OldaleTown_PokemonCenter_1F_EventScript_Gentleman::
	msgbox OldaleTown_PokemonCenter_1F_Text_TrainersCanUsePC, MSGBOX_NPC
	end

OldaleTown_PokemonCenter_1F_EventScript_Boy::
	msgbox OldaleTown_PokemonCenter_1F_Text_PokemonCentersAreGreat, MSGBOX_NPC
	end

OldaleTown_PokemonCenter_1F_EventScript_Girl::
	lock
	faceplayer
	goto_if_set FLAG_SYS_POKEDEX_GET, OldaleTown_PokemonCenter_1F_EventScript_WirelessClubAvailable
	msgbox OldaleTown_PokemonCenter_1F_Text_WirelessClubNotAvailable, MSGBOX_DEFAULT
	release
	end

OldaleTown_PokemonCenter_1F_EventScript_WirelessClubAvailable::
	msgbox OldaleTown_PokemonCenter_1F_Text_TradedInWirelessClub, MSGBOX_DEFAULT
	release
	end

OldaleTown_PokemonCenter_1F_Text_TrainersCanUsePC:
	.string "Köşedeki bilgisayarı isteyen tüm\neğitmenler kullanabilir.\pDoğal olarak bu, senin de onu\nkullanabileceğin anlamına geliyor.$"

OldaleTown_PokemonCenter_1F_Text_PokemonCentersAreGreat:
	.string "POKéMON MERKEZ'leri harikadır!\pHizmetlerinden istediğin kadar\nyararlanabilirsin ve hepsi\lücretsizdir. Asla endişelenmenize gerek\lolmaz!$"

OldaleTown_PokemonCenter_1F_Text_WirelessClubNotAvailable:
	.string "İkinci kattaki POKéMON KABLOSUZ\nKULÜBÜ yakın zamanda inşa edildi.\pAma hâlâ ayarlamalar yaptıklarını\nsöylüyorlar.$"

OldaleTown_PokemonCenter_1F_Text_TradedInWirelessClub:
	.string "İkinci kattaki POKéMON KABLOSUZ KULÜBÜ\nyakın zamanda inşa edildi.\pHemen POKéMON takası yaptım.$"


