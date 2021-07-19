#include "pch.h"
#include "gameclasses.h"

namespace GameGlobals
{
	bool IsInitialized = false;
	CHostState* HostState = nullptr;
	CInputSystem* InputSystem = nullptr;

	void InitGameGlobals()
	{
		HostState = reinterpret_cast<CHostState*>(0x141736120); // Get CHostState from memory.
		InputSystem = *reinterpret_cast<CInputSystem**>(0x14D40B380); // Get IInputSystem from memory.

		IsInitialized = true;
	}
}