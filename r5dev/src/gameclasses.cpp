#include "pch.h"
#include "gameclasses.h"
#include "id3dx.h"
#include "cgameconsole.h"
#include "squirrel.h"
#include <r5net.h>

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
	CClient* Client = nullptr;
	BanList* BanSystem = new BanList();

	CKeyValuesSystem* KeyValuesSystem = nullptr;
	KeyValues** PlaylistKeyValues = nullptr;	

	std::vector<std::string> allPlaylists = { "none" };

	namespace CustomCommandVariations
	{
		void CGameConsole_Callback(const CCommand& cmd)
		{
			g_bShowConsole = !g_bShowConsole;
		}

		void CCompanion_Callback(const CCommand& cmd)
		{
			g_bShowBrowser = !g_bShowBrowser;
		}

		void Kick_Callback(CCommand* cmd)
		{
			std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
			if (argSize < 2) // Do we atleast have 2 arguments?
				return;

			CCommand& cmdReference = *cmd; // Get reference.
			const char* firstArg = cmdReference[1]; // Get first arg.

			for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
			{
				CClient* client = GameGlobals::Client->GetClientInstance(i); // Get client instance.
				if (!client)
					continue;

				if (!client->GetNetChan()) // Netchan valid?
					continue;

				void* clientNamePtr = (void**)(((std::uintptr_t)client->GetNetChan()) + 0x1A8D); // Get client name from netchan.
				std::string clientName((char*)clientNamePtr, 32); // Get full name.

				if (clientName.empty()) // Empty name?
					continue;

				if (strcmp(firstArg, clientName.c_str()) != 0) // Our wanted name?
					continue;

				DisconnectClient(client, "Kicked from Server", 0, 1); // Disconnect client.
			}
		}

		void KickID_Callback(CCommand* cmd)
		{
			static auto HasOnlyDigits = [](const std::string& string)
			{
				for (const char& character : string)
				{
					if (std::isdigit(character) == 0)
						return false;
				}
				return true;
			};

			std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
			if (argSize < 2) // Do we atleast have 2 arguments?
				return;

			CCommand& cmdReference = *cmd; // Get reference.
			std::string firstArg = cmdReference[1]; // Get first arg.

			try
			{
				bool onlyDigits = HasOnlyDigits(firstArg); // Only has digits?
				for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
				{
					CClient* client = GameGlobals::Client->GetClientInstance(i); // Get client instance.
					if (!client)
						continue;

					if (!client->GetNetChan()) // Netchan valid?
						continue;

					std::string finalIPAddress = "null"; // If this stays null they modified the packet somehow.
					MemoryAddress ipAddressField = MemoryAddress(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
					if (ipAddressField)
					{
						std::stringstream ss;
						ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

						finalIPAddress = ss.str();
					}

					if (onlyDigits)
					{
						std::int64_t ID = static_cast<std::int64_t>(std::stoll(firstArg));
						if (ID > MAX_PLAYERS) // Is it a possible originID?
						{
							std::int64_t originID = client->m_iOriginID;
							if (originID != ID) // See if they match.
								continue;
						}
						else // If its not try by userID.
						{
							std::int64_t clientID = static_cast<std::int64_t>(client->m_iUserID + 1); // Get UserID + 1.
							if (clientID != ID) // See if they match.
								continue;
						}

						DisconnectClient(client, "Kicked from Server", 0, 1); // Disconnect client.
					}
					else
					{
						if (firstArg.compare(finalIPAddress) != NULL) // Do the string equal?
							continue;

						DisconnectClient(client, "Kicked from Server", 0, 1); // Disconnect client.
					}
				}
			}
			catch (std::exception& e)
			{
				spdlog::critical("Kick UID asked for a userID or originID :( You can get the userid with the 'status' command. Error: {}\n", e.what());
				g_GameConsole->AddLog("Kick UID asked for a userID or originID :( You can get the userid with the 'status' command. Error: %s", e.what());
				return;
			}
		}

		void Unban_Callback(CCommand* cmd)
		{
			static auto HasOnlyDigits = [](const std::string& string)
			{
				for (const char& character : string)
				{
					if (std::isdigit(character) == 0)
						return false;
				}
				return true;
			};

			std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
			if (argSize < 2) // Do we atleast have 2 arguments?
				return;

			CCommand& cmdReference = *cmd; // Get reference.

			try
			{
				const char* firstArg = cmdReference[1];
				if (HasOnlyDigits(firstArg)) // Check if we have an ip address or origin ID.
				{
					GameGlobals::BanSystem->DeleteEntry("noIP", std::stoll(firstArg)); // Delete ban entry.
					GameGlobals::BanSystem->Save(); // Save modified vector to file.
				}
				else
				{
					GameGlobals::BanSystem->DeleteEntry(firstArg, 1); // Delete ban entry.
					GameGlobals::BanSystem->Save(); // Save modified vector to file.
				}
			}
			catch (std::exception& e)
			{
				spdlog::critical("Unban Error: {}\n", e.what());
				g_GameConsole->AddLog("Unban Error: %s", e.what());
				return;
			}
		}

		void ReloadBanList_Callback(CCommand* cmd)
		{
			GameGlobals::BanSystem->Load(); // Reload banlist.
		}

		void Ban_Callback(CCommand* cmd)
		{
			std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
			if (argSize < 2) // Do we atleast have 2 arguments?
				return;

			CCommand& cmdReference = *cmd; // Get reference.
			const char* firstArg = cmdReference[1]; // Get first arg.

			for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
			{
				CClient* client = GameGlobals::Client->GetClientInstance(i); // Get client instance.
				if (!client)
					continue;

				if (!client->GetNetChan()) // Netchan valid?
					continue;

				void* clientNamePtr = (void**)(((std::uintptr_t)client->GetNetChan()) + 0x1A8D); // Get client name from netchan.
				std::string clientName((char*)clientNamePtr, 32); // Get full name.

				if (clientName.empty()) // Empty name?
					continue;

				if (strcmp(firstArg, clientName.c_str()) != 0) // Our wanted name?
					continue;

				std::string finalIPAddress = "null"; // If this stays null they modified the packet somehow.
				MemoryAddress ipAddressField = MemoryAddress(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
				if (ipAddressField && ipAddressField.GetValue<int>() != 0x0)
				{
					std::stringstream ss;
					ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

					finalIPAddress = ss.str();
				}

				GameGlobals::BanSystem->AddEntry(finalIPAddress, client->m_iOriginID); // Add ban entry.
				GameGlobals::BanSystem->Save(); // Save ban list.
				DisconnectClient(client, "Banned from Server", 0, 1); // Disconnect client.
			}
		}

		void BanID_Callback(CCommand* cmd)
		{
			static auto HasOnlyDigits = [](const std::string& string)
			{
				for (const char& character : string)
				{
					if (std::isdigit(character) == 0)
						return false;
				}
				return true;
			};

			std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
			if (argSize < 2) // Do we atleast have 2 arguments?
				return;

			CCommand& cmdReference = *cmd; // Get reference.
			std::string firstArg = cmdReference[1];

			try
			{
				bool onlyDigits = HasOnlyDigits(firstArg); // Only has digits?
				for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
				{
					CClient* client = GameGlobals::Client->GetClientInstance(i); // Get client instance.
					if (!client)
						continue;

					if (!client->GetNetChan()) // Netchan valid?
						continue;

					std::string finalIPAddress = "null"; // If this stays null they modified the packet somehow.
					MemoryAddress ipAddressField = MemoryAddress(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
					if (ipAddressField)
					{
						std::stringstream ss;
						ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
							<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

						finalIPAddress = ss.str();
					}

					if (onlyDigits)
					{
						std::int64_t ID = static_cast<std::int64_t>(std::stoll(firstArg));
						if (ID > MAX_PLAYERS) // Is it a possible originID?
						{
							std::int64_t originID = client->m_iOriginID;
							if (originID != ID) // See if they match.
								continue;
						}
						else // If its not try by userID.
						{
							std::int64_t clientID = static_cast<std::int64_t>(client->m_iUserID + 1); // Get UserID + 1.
							if (clientID != ID) // See if they match.
								continue;
						}

						GameGlobals::BanSystem->AddEntry(finalIPAddress, client->m_iOriginID); // Add ban entry.
						GameGlobals::BanSystem->Save(); // Save ban list.
						DisconnectClient(client, "Banned from Server", 0, 1); // Disconnect client.
					}
					else
					{
						if (firstArg.compare(finalIPAddress) != NULL) // Do the string equal?
							continue;

						GameGlobals::BanSystem->AddEntry(finalIPAddress, client->m_iOriginID); // Add ban entry.
						GameGlobals::BanSystem->Save(); // Save ban list.
						DisconnectClient(client, "Banned from Server", 0, 1); // Disconnect client.
					}
				}
			}
			catch (std::exception& e)
			{
				spdlog::critical("Banid Error: {}\n", e.what());
				g_GameConsole->AddLog("Banid Error: %s", e.what());
				return;
			}
		}
	}

	void NullHostNames()
	{
		spdlog::debug("Nulling host names..\n");
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
		spdlog::debug("Initializing Game Globals..\n");
		HostState = reinterpret_cast<CHostState*>(0x141736120); // Get CHostState from memory.
		InputSystem = *reinterpret_cast<CInputSystem**>(0x14D40B380); // Get IInputSystem from memory.
		Cvar = *reinterpret_cast<CCVar**>(0x14D40B348); // Get CCVar from memory.
		KeyValuesSystem = reinterpret_cast<CKeyValuesSystem*>(0x141F105C0); // Get CKeyValuesSystem from memory.
		PlaylistKeyValues = reinterpret_cast<KeyValues**>(0x16705B980); // Get the KeyValue for the playlist file.
		Client = reinterpret_cast<CClient*>(0x16073B200);

		NullHostNames(); // Null all hostnames.
		InitAllCommandVariations(); // Initialize our custom ConVars.
		*(char*)addr_m_bRestrictServerCommands = true; // Restrict commands.
		void* disconnect = Cvar->FindCommand("disconnect");
		*(std::int32_t*)((std::uintptr_t)disconnect + 0x38) |= FCVAR_SERVER_CAN_EXECUTE; // Make sure server is not restricted to this.

		std::thread t1(InitPlaylist); // Start thread to grab playlists.
		t1.detach(); // Detach thread from current one.

		IsInitialized = true;
	}
	
	void InitPlaylist()
	{
		spdlog::debug("Parsing Playlist..\n");
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

	void InitAllCommandVariations()
	{
		spdlog::debug("Initializing all Custom ConVars and Commands..\n");
		void* CGameConsoleConCommand =  CreateCustomConCommand("cgameconsole", "Opens the R5 Reloaded Console.", 0, CustomCommandVariations::CGameConsole_Callback, nullptr);
		void* CCompanionConCommand =    CreateCustomConCommand("ccompanion", "Opens the R5 Reloaded Server Browser.", 0, CustomCommandVariations::CCompanion_Callback, nullptr);
		void* KickConCommand =          CreateCustomConCommand("kick", "Kick a client from the Server via name. | Usage: kick (name).", 0, CustomCommandVariations::Kick_Callback, nullptr);
		void* KickIDConCommand =        CreateCustomConCommand("kickid", "Kick a client from the Server via userID or originID | Usage: kickid (originID/userID)", 0, CustomCommandVariations::KickID_Callback, nullptr);
		void* UnbanConCommand =         CreateCustomConCommand("unban", "Unbans a client from the Server via IP or originID | Usage: unban (originID/ipAddress)", 0, CustomCommandVariations::Unban_Callback, nullptr);
		void* ReloadBanListConCommand = CreateCustomConCommand("reloadbanlist", "Reloads the ban list from disk.", 0, CustomCommandVariations::ReloadBanList_Callback, nullptr);
		void* BanConCommand =           CreateCustomConCommand("ban", "Bans a client from the Server via name. | Usage: ban (name)", 0, CustomCommandVariations::Ban_Callback, nullptr);
		void* BanIDConCommand =         CreateCustomConCommand("banid", "Bans a client from the Server via originID, userID or IP | Usage: banid (originID/ipAddress/userID)", 0, CustomCommandVariations::BanID_Callback, nullptr);
		
		ConVar* DrawConsoleOverlayConVar = CreateCustomConVar("cl_drawconsoleoverlay", "0", 0, "Draw the console overlay at the top of the screen", false, 0.f, false, 0.f, nullptr, nullptr);
	}

	void* CreateCustomConCommand(const char* name, const char* helpString, int flags, void* callback, void* callbackAfterExecution)
	{
		static MemoryAddress ConCommandVtable = MemoryAddress(0x14136BD70);
		static MemoryAddress NullSub = MemoryAddress(0x1401B3280);
		static MemoryAddress CallbackCompletion = MemoryAddress(0x1401E3990);
		static MemoryAddress RegisterConCommand = MemoryAddress(0x14046F470);

		void* command = reinterpret_cast<void*>(addr_MemAlloc_Wrapper(0x68)); // Allocate new memory with StdMemAlloc else we crash.
		memset(command, 0, 0x68); // Set all to null.
		std::uintptr_t commandPtr = reinterpret_cast<std::uintptr_t>(command); // To ptr.

		*(void**)commandPtr = ConCommandVtable.RCast<void*>(); // 0x0 to ConCommand vtable.
		*(const char**)(commandPtr + 0x18) = name; // 0x18 to ConCommand Name.
		*(const char**)(commandPtr + 0x20) = helpString; // 0x20 to ConCommand help string.
		*(std::int32_t*)(commandPtr + 0x38) = flags; // 0x38 to ConCommand Flags.
		*(void**)(commandPtr + 0x40) = NullSub.RCast<void*>(); // 0x40 Nullsub since every concommand has it.
		*(void**)(commandPtr + 0x50) = callback; // 0x50 has function callback.
		*(std::int32_t*)(commandPtr + 0x60) = 2; // 0x60 Set to use callback and newcommand callback.

		if (callbackAfterExecution) // Do we wanna have a callback after execution?
		{
			*(void**)(commandPtr + 0x58) = callbackAfterExecution; // 0x58 to our callback after execution.
		}
		else
		{
			*(void**)(commandPtr + 0x58) = CallbackCompletion.RCast<void*>(); // 0x58 nullsub.
		}

		RegisterConCommand.RCast<void(*)(void*)>()((void*)commandPtr); // Register command in ConVarAccessor.

		return command;
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
		*(void**)cvarPtr = ConVarVtable.RCast<void*>(); // 0x0 to ConVar vtable.

		CreateConVar.RCast<void(*)(ConVar*, const char*, const char*, int, const char*, bool, float, bool, float, void*, void*)>() // Call to create ConVar.
			(allocatedConvar, name, defaultValue, flags, helpString, bMin, fMin, bMax, fMax, callback, unk);

		return allocatedConvar; // Return allocated ConVar.
	}

	void Script_RegisterFunction(void* sqvm, const char* name, const char* helpString, const char* retValType, const char* argTypes, void* funcPtr)
	{
		static MemoryAddress Script_RegisterFunction = MemoryAddress(0x141056040);

		SQFuncRegistration* func = new SQFuncRegistration();

		func->scriptName = name;
		func->nativeName = name;
		func->helpString = helpString;
		func->retValType = retValType;
		func->argTypes   = argTypes;
		func->funcPtr    = funcPtr;

		Script_RegisterFunction.RCast<void(*)(void*, SQFuncRegistration*, char unk)>()(sqvm, func, 1);
	}

	int Script_NativeTest(void* sqvm)
	{
		// function code goes here

		return 1;
	}

	void RegisterUIScriptFunctions(void* sqvm)
	{
		//Script_RegisterFunction(sqvm, "UINativeTest", "native ui function", "void", "", &Script_NativeTest);
	}

	void RegisterClientScriptFunctions(void* sqvm)
	{
		//Script_RegisterFunction(sqvm, "ClientNativeTest", "native client function", "void", "", &Script_NativeTest);
	}

	void RegisterServerScriptFunctions(void* sqvm)
	{
		//Script_RegisterFunction(sqvm, "ServerNativeTest", "native server function", "void", "", &Script_NativeTest);
	}

	void DisconnectClient(CClient* client, const char* reason, unsigned __int8 unk1, char unk2)
	{
		if (!client) //	Client valid?
			return;

		if (std::strlen(reason) == NULL) // Is reason null?
			return;

		if (!client->GetNetChan())
			return;

		addr_NetChan_Shutdown(client->GetNetChan(), reason, unk1, unk2); // Shutdown netchan.
		client->GetNetChan() = nullptr; // Null netchan.
		MemoryAddress(0x140302FD0).RCast<void(*)(CClient*)>()(client); // Reset CClient instance for client.
	}
}

#pragma region KeyValues
const char* KeyValues::GetName()
{
	return GameGlobals::KeyValuesSystem->GetStringForSymbol(MAKE_3_BYTES_FROM_1_AND_2(m_iKeyNameCaseSensitive, m_iKeyNameCaseSensitive2));
}
#pragma endregion