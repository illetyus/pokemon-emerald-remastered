Route111_Text_MakingRoomUseTMToMakeYourOwn:
	.string "Ne oldu? Ne mi yapıyorum?\pPOKéMON saldırısı kullanarak kendi\nodamı yapmayı düşünüyorum.\pEvet, bu mümkün!\nSana bu TM'yi vereyim.\pKendi odanı yapmak için kullanacak\nmısın?$"

Route111_Text_ExplainSecretPower:
	.string "Sarmaşık düşürebilecek gibi görünen\nbüyük bir ağaç bul.\pAğacın önünde GİZLİ GÜÇ'ü kullan.\nSarmaşıklar kalıp aşağı düşecektir,\lböylece ağaca tırmanabilirsin.\pEn sevdiğin şeyleri koymak için çok\nfazla alan bulacaksınız.\pKendi gizli odan olacak…\nGİZLİ BİR ÜS!\pSen de hemen denemelisin.\pBir şey daha, GİZLİ ÜS'ünü her zaman\nbir ağaçta yapmak zorunda değilsin.\pGİZLİ GÜÇ'ü küçük girintileri olan\nkaya duvarlarında kullanmayı dene.\pBen başka yerlere de bakacağım.\nTamam, hoşça kal!$"

Route111_Text_DontWantThis:
	.string "Ha, bunu istemiyor musun?\nFikrini değiştirirsen bana söyle,\ltamam mı?$"

Route111_Text_DontHaveAnyRoom:
	.string "Bunun için yerin kalmamış.\pSenin için tutacağım. Başka zaman\ngel, tamam mı?$"


Route111_EventScript_SecretPowerMan::
	lock
	faceplayer
	msgbox Route111_Text_MakingRoomUseTMToMakeYourOwn, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, Route111_EventScript_GiveSecretPower
	msgbox Route111_Text_DontWantThis, MSGBOX_DEFAULT
	release
	end

Route111_EventScript_GiveSecretPower::
	giveitem ITEM_TM43
	goto_if_eq VAR_RESULT, FALSE, Route111_EventScript_NoRoomForSecretPower
	msgbox Route111_Text_ExplainSecretPower, MSGBOX_DEFAULT
	closemessage
	setflag FLAG_RECEIVED_SECRET_POWER
	clearflag FLAG_HIDE_SLATEPORT_CITY_TM_SALESMAN
	call_if_eq VAR_FACING, DIR_WEST, Route111_EventScript_SecretPowerManExit
	call_if_eq VAR_FACING, DIR_EAST, Route111_EventScript_SecretPowerManExit
	call_if_eq VAR_FACING, DIR_NORTH, Route111_EventScript_SecretPowerManExitNorth
	removeobject VAR_LAST_TALKED
	release
	end

Route111_EventScript_SecretPowerManExit::
	applymovement VAR_LAST_TALKED, Route111_Movement_SecretPowerManExit
	waitmovement 0
	return

Route111_EventScript_SecretPowerManExitNorth::
	applymovement VAR_LAST_TALKED, Route111_Movement_SecretPowerManExitNorth
	waitmovement 0
	return

Route111_EventScript_NoRoomForSecretPower::
	msgbox Route111_Text_DontHaveAnyRoom, MSGBOX_DEFAULT
	release
	end

Route111_Movement_SecretPowerManExit:
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	step_end

Route111_Movement_SecretPowerManExitNorth:
	walk_left
	walk_down
	walk_down
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_down
	walk_down
	step_end
