.set LOCALID_NURSE, 1

RustboroCity_PokemonCenter_1F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, RustboroCity_PokemonCenter_1F_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CableClub_OnResume
	.byte 0

RustboroCity_PokemonCenter_1F_OnTransition:
	setrespawn HEAL_LOCATION_RUSTBORO_CITY
	call Common_EventScript_UpdateBrineyLocation
	end

RustboroCity_PokemonCenter_1F_EventScript_Nurse::
	setvar VAR_0x800B, LOCALID_NURSE
	call Common_EventScript_PkmnCenterNurse
	waitmessage
	waitbuttonpress
	release
	end

RustboroCity_PokemonCenter_1F_EventScript_Man::
	msgbox RustboroCity_PokemonCenter_1F_Text_PokemonHavePersonalities, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_EventScript_Boy::
	msgbox RustboroCity_PokemonCenter_1F_Text_MaleAndFemalePokemon, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_EventScript_Girl::
	msgbox RustboroCity_PokemonCenter_1F_Text_HMCutNextDoor, MSGBOX_NPC
	end

RustboroCity_PokemonCenter_1F_Text_PokemonHavePersonalities:
	.string "POKéMON'um NAIVE doğasına sahip.\nArkadaşımınki ise JOLLY doğasına\lsahip.\pPOKéMON'un kişiliklerinin olması\nbüyüleyici!$"

RustboroCity_PokemonCenter_1F_Text_MaleAndFemalePokemon:
	.string "Tıpkı insanlar gibi, erkek ve dişi\nPOKéMON'lar da var.\pAncak hiç kimse nasıl farklı oldukları\nhakkında bir fikre sahip değil gibi\lgörünüyor.$"

RustboroCity_PokemonCenter_1F_Text_HMCutNextDoor:
	.string "Yandaki adam bana bir HM verdi!\pOnu POKéMON'uma ince ağaçları nasıl\nkeseceğini öğretmek için kullandım.$"


