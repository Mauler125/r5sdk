#include "pch.h"
#include "gameclasses.h"
#include "id3dx.h"

//  Need this for a re-factor later.
//	Interface* interfaces = *reinterpret_cast<Interface**>(0x167F4FA48);

//	for (Interface* current = interfaces; current; current = reinterpret_cast<Interface*>(current->NextInterfacePtr))
//	{
//		printf("%s: %p\n", current->InterfaceName, current->InterfacePtr);
//	}

namespace GameGlobals
{
	bool IsInitialized = false;
	CHostState* HostState = nullptr;
	CInputSystem* InputSystem = nullptr;
	CCVar* Cvar = nullptr;

	CKeyValuesSystem* KeyValuesSystem = nullptr;
	KeyValues** PlaylistKeyValues = nullptr;	

	std::vector<std::string> allPlaylists = { "none" };

	namespace CustomConVar
	{
		void CGameConsole_Callback(void* cmd)
		{
			std::string szValue = *(const char**)((std::uintptr_t)cmd + 0x18);
			try
			{
				int value = std::stoi(szValue);
				switch (value)
				{
				case 0:
					g_bShowConsole = false;
					break;
				case 1:
					g_bShowConsole = true;
					break;
				default:
					break;
				}
			}
			catch (std::exception& e)
			{
				std::cout << " [+CGameConsole+] Please don't input a character that isn't a number into cgameconsole :(. Error: " << e.what() << std::endl;
			}
		}

		void CCompanion_Callback(void* cmd)
		{
			std::string szValue = *(const char**)((std::uintptr_t)cmd + 0x18);
			try
			{
				int value = std::stoi(szValue);
				switch (value)
				{
				case 0:
					g_bShowBrowser = false;
					break;
				case 1:
					g_bShowBrowser = true;
					break;
				default:
					break;
				}
			}
			catch (std::exception& e)
			{
				std::cout << " [+CCompanion+] Please don't input a character that isn't a number into ccompanion :(. Error: " << e.what() << std::endl;
			};
		}
	}

	void NullHostNames()
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
		KeyValuesSystem = reinterpret_cast<CKeyValuesSystem*>(0x141F105C0); // Get CKeyValuesSystem from memory.
		PlaylistKeyValues = reinterpret_cast<KeyValues**>(0x16705B980); // Get the KeyValue for the playlist file.

		NullHostNames(); // Null all hostnames.
		InitConVars(); // Initialize our custom ConVars.

		std::thread t1(InitPlaylist); // Start thread to grab playlists.
		t1.detach(); // Detach thread from current one.

		IsInitialized = true;
	}
	
	void InitPlaylist()
	{
		while (true)
		{
			if ((*PlaylistKeyValues))
			{
				KeyValues* playlists = (*PlaylistKeyValues)->FindKey("Playlists", false); // Find playlists key.
				if (playlists)
				{
					allPlaylists.clear();
					for (KeyValues* dat = playlists->m_pSub; dat != nullptr; dat = dat->m_pPeer) // Parse through all sub keys.
					{
						allPlaylists.push_back(dat->GetName()); // Get all playlist names.
					}

					break; // Break if playlist got filled.
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	void InitConVars()
	{
		ConVar* CGameConsoleConVar = CreateCustomConVar("cgameconsole", "0", 0, "Opens the R5 Reloaded Console", false, 0.f, false, 0.f, CustomConVar::CGameConsole_Callback, nullptr);
		ConVar* CCompanionConVar = CreateCustomConVar("ccompanion", "0", 0, "Opens the R5 Reloaded Server Browser", false, 0.f, false, 0.f, CustomConVar::CCompanion_Callback, nullptr);
	}

	ConVar* CreateCustomConVar(const char* name, const char* defaultValue, int flags, const char* helpString, bool bMin, float fMin, bool bMax, float fMax, void* callback, void* unk)
	{
		static MemoryAddress ConVarVtable = MemoryAddress(0x14046FB50).Offset(0x12).ResolveRelativeAddress(); // Get vtable ptr for ConVar table.
		static MemoryAddress ICvarVtable = MemoryAddress(0x14046FB50).Offset(0x29).ResolveRelativeAddress(); // Get vtable ptr for ICvar table.
		static MemoryAddress CreateConVar = MemoryAddress(0x140470540); // Get CreateConvar address.

		ConVar* allocatedConvar = reinterpret_cast<ConVar*>(addr_MemAlloc_Wrapper(0xA0)); // Allocate new memory with StdMemAlloc else we crash.
		memset(allocatedConvar, 0, 0xA0); // Set all to null.
		std::uintptr_t cvarPtr = reinterpret_cast<std::uintptr_t>(allocatedConvar); // To ptr.

		*(void**)(cvarPtr + 0x40) = ICvarVtable.RCast<void*>(); // 0x40 to ICvar table.
		*(void**)cvarPtr = ConVarVtable.RCast<void*>(); // 0x0 to ConVar table.

		CreateConVar.RCast<void(*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, void*, void*)>() // Call to create ConVar.
			(allocatedConvar, name, defaultValue, flags, helpString, bMin, fMin, bMax, fMax, callback, unk);

		return allocatedConvar; // Return allocated ConVar.
	}
}

#pragma region KeyValues

const char* KeyValues::GetName()
{
	return GameGlobals::KeyValuesSystem->GetStringForSymbol(MAKE_3_BYTES_FROM_1_AND_2(m_iKeyNameCaseSensitive, m_iKeyNameCaseSensitive2));
}

#pragma endregion