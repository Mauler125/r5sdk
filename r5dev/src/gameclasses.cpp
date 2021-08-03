#include "pch.h"
#include "gameclasses.h"

namespace GameGlobals
{
	bool IsInitialized = false;
	CHostState* HostState = nullptr;
	CInputSystem* InputSystem = nullptr;
	CCVar* Cvar = nullptr;

	void InitGameGlobals()
	{
		HostState = reinterpret_cast<CHostState*>(0x141736120); // Get CHostState from memory.
		InputSystem = *reinterpret_cast<CInputSystem**>(0x14D40B380); // Get IInputSystem from memory.
		Cvar = *reinterpret_cast<CCVar**>(0x14D40B348); // Get CCVar from memory.
	//	Interface* interfaces = *reinterpret_cast<Interface**>(0x167F4FA48);

	//	for (Interface* current = interfaces; current; current = reinterpret_cast<Interface*>(current->NextInterfacePtr))
	//	{
	//		printf("%s: %p\n", current->InterfaceName, current->InterfacePtr);
	//	}

		IsInitialized = true;
	}
}