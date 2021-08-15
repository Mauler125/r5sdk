#include "pch.h"
#include "gameclasses.h"

namespace GameGlobals
{
	bool IsInitialized = false;
	CHostState* HostState = nullptr;
	CInputSystem* InputSystem = nullptr;
	CCVar* Cvar = nullptr;

	void EmptyHostNames()
	{
		const char* hostnameArray[] =
		{
			"pin_telemetry_hostname",
			"assetdownloads_hostname",
			"users_hostname",
			"persistence_hostname",
			"speechtotexttoken_hostname",
			"communities_hostname",
			"persistenceDef_hostname",
			"party_hostname",
			"speechtotext_hostname",
			"serverReports_hostname",
			"subscription_hostname",
			"steamlink_hostname",
			"staticfile_hostname",
			"matchmaking_hostname",
			"skill_hostname",
			"publication_hostname",
			"stats_hostname"
		};

		for (int i = 0; i < 17; i++)
		{
			const char* name = hostnameArray[i];
			Cvar->FindVar(name)->m_pzsCurrentValue = "0.0.0.0";
		}
	}

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
		EmptyHostNames();
		IsInitialized = true;
	}
}