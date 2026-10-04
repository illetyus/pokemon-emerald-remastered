PacifidlogTown_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, PacifidlogTown_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, PacifidlogTown_OnResume
	.byte 0

PacifidlogTown_OnTransition:
	setflag FLAG_VISITED_PACIFIDLOG_TOWN
	end

PacifidlogTown_OnResume:
	setstepcallback STEP_CB_PACIFIDLOG_BRIDGE
	end

PacifidlogTown_EventScript_NinjaBoy::
	msgbox PacifidlogTown_Text_NeatHousesOnWater, MSGBOX_NPC
	end

PacifidlogTown_EventScript_Girl::
	msgbox PacifidlogTown_Text_FastRunningCurrent, MSGBOX_NPC
	end

PacifidlogTown_EventScript_Fisherman::
	msgbox PacifidlogTown_Text_SkyPillarTooScary, MSGBOX_NPC
	end

PacifidlogTown_EventScript_TownSign::
	msgbox PacifidlogTown_Text_TownSign, MSGBOX_SIGN
	end

PacifidlogTown_Text_FastRunningCurrent:
	.string "PACİFİDLOG ve SLATEPORT arasındaki\ndenizde hızlı bir gelgit vardır.\pSörf yapmaya karar verirsen, başka\nbir yere sürüklenebilirsin.$"

PacifidlogTown_Text_NeatHousesOnWater:
	.string "Gördün mü, ne kadar güzel, değil mi?\nBu evler suyun üzerinde!\pBen burada doğdum!$"

PacifidlogTown_Text_SkyPillarTooScary:
	.string "GÖK KULESİ mi?\p…Ah, biraz ilerideki uzun, yüksek\nkuleyi kastediyor olmalısın.\pBana sorsaydın, oraya tırmanmazdım.\nO kadar yükseğe çıkmak çok korkutucu.\pPACİFİDLOG'da deniz seviyesinde\nyaşamak bana uyar.$"

PacifidlogTown_Text_TownSign:
	.string "PACİFİDLOG KÖYÜ\pSabah güneşinin suya gülümsediği yer.$"


