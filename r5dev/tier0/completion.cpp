//=============================================================================//
//
// Purpose: Completion functions for ConCommand callbacks.
//
//=============================================================================//

#include "core/stdafx.h"
#include "windows/id3dx.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "tier0/completion.h"
#ifndef DEDICATED
#include "engine/cl_rcon.h"
#endif // !DEDICATED
#include "engine/net_chan.h"
#include "engine/sys_utils.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "vpklib/packedstore.h"
#ifndef DEDICATED
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#include "public/include/bansystem.h"
#include "mathlib/crc32.h"

#ifndef DEDICATED
/*
=====================
_CGameConsole_f_CompletionFunc
=====================
*/
void _CGameConsole_f_CompletionFunc(const CCommand& cmd)
{
	g_pIConsole->m_bActivate = !g_pIConsole->m_bActivate;
}

/*
=====================
_CCompanion_f_CompletionFunc
=====================
*/
void _CCompanion_f_CompletionFunc(const CCommand& cmd)
{
	g_pIBrowser->m_bActivate = !g_pIBrowser->m_bActivate;
}
#endif // !DEDICATED

/*
=====================
_Kick_f_CompletionFunc
=====================
*/
void _Kick_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	const char* firstArg = args[1]; // Get first arg.

	for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
	{
		CClient* client = g_pClient->GetClientInstance(i); // Get client instance.
		if (!client)
		{
			continue;
		}

		if (!client->GetNetChan()) // Netchan valid?
		{
			continue;
		}

		void* clientNamePtr = (void**)(((std::uintptr_t)client->GetNetChan()) + 0x1A8D); // Get client name from netchan.
		std::string clientName((char*)clientNamePtr, 32); // Get full name.

		if (clientName.empty()) // Empty name?
		{
			continue;
		}

		if (strcmp(firstArg, clientName.c_str()) != 0) // Our wanted name?
		{
			continue;
		}

		NET_DisconnectClient(client, i, "Kicked from Server", 0, 1); // Disconnect client.
	}
}

/*
=====================
_KickID_f_CompletionFunc
=====================
*/
void _KickID_f_CompletionFunc(CCommand* cmd)
{
	static auto HasOnlyDigits = [](const std::string& string)
	{
		for (const char& character : string)
		{
			if (std::isdigit(character) == 0)
			{
				return false;
			}
		}
		return true;
	};

	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	std::string firstArg = args[1]; // Get first arg.

	try
	{
		bool onlyDigits = HasOnlyDigits(firstArg); // Only has digits?
		for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
		{
			CClient* client = g_pClient->GetClientInstance(i); // Get client instance.
			if (!client)
			{
				continue;
			}

			if (!client->GetNetChan()) // Netchan valid?
			{
				continue;
			}

			std::string finalIpAddress = "null"; // If this stays null they modified the packet somehow.
			ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
			if (ipAddressField)
			{
				std::stringstream ss;
				ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

				finalIpAddress = ss.str();
			}

			if (onlyDigits)
			{
				std::int64_t ID = static_cast<std::int64_t>(std::stoll(firstArg));
				if (ID > MAX_PLAYERS) // Is it a possible originID?
				{
					std::int64_t originID = client->m_iOriginID;
					if (originID != ID) // See if they match.
					{
						continue;
					}
				}
				else // If its not try by userID.
				{
					std::int64_t clientID = static_cast<std::int64_t>(client->m_iUserID + 1); // Get UserID + 1.
					if (clientID != ID) // See if they match.
					{
						continue;
					}
				}

				NET_DisconnectClient(client, i, "Kicked from Server", 0, 1); // Disconnect client.
			}
			else
			{
				if (firstArg.compare(finalIpAddress) != NULL) // Do the string equal?
				{
					continue;
				}

				NET_DisconnectClient(client, i, "Kicked from Server", 0, 1); // Disconnect client.
			}
		}
	}
	catch (std::exception& e)
	{
		DevMsg(eDLL_T::SERVER, "sv_kickid requires a UserID or OriginID. You can get the UserID with the 'status' command. Error: %s", e.what());
		return;
	}
}

/*
=====================
_Ban_f_CompletionFunc
=====================
*/
void _Ban_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	const char* firstArg = args[1]; // Get first arg.

	for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
	{
		CClient* client = g_pClient->GetClientInstance(i); // Get client instance.
		if (!client)
		{
			continue;
		}

		if (!client->GetNetChan()) // Netchan valid?
		{
			continue;
		}

		void* clientNamePtr = (void**)(((std::uintptr_t)client->GetNetChan()) + 0x1A8D); // Get client name from netchan.
		std::string clientName((char*)clientNamePtr, 32); // Get full name.

		if (clientName.empty()) // Empty name?
		{
			continue;
		}

		if (strcmp(firstArg, clientName.c_str()) != 0) // Our wanted name?
		{
			continue;
		}

		std::string finalIpAddress = "null"; // If this stays null they modified the packet somehow.
		ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
		if (ipAddressField && ipAddressField.GetValue<int>() != 0x0)
		{
			std::stringstream ss;
			ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

			finalIpAddress = ss.str();
		}

		g_pBanSystem->AddEntry(finalIpAddress, client->m_iOriginID); // Add ban entry.
		g_pBanSystem->Save(); // Save ban list.
		NET_DisconnectClient(client, i, "Banned from Server", 0, 1); // Disconnect client.
	}
}

/*
=====================
_BanID_f_CompletionFunc
=====================
*/
void _BanID_f_CompletionFunc(CCommand* cmd)
{
	static auto HasOnlyDigits = [](const std::string& string)
	{
		for (const char& character : string)
		{
			if (std::isdigit(character) == 0)
			{
				return false;
			}
		}
		return true;
	};

	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	std::string firstArg = args[1];

	try
	{
		bool onlyDigits = HasOnlyDigits(firstArg); // Only has digits?
		for (int i = 0; i < MAX_PLAYERS; i++) // Loop through all possible client instances.
		{
			CClient* client = g_pClient->GetClientInstance(i); // Get client instance.
			if (!client)
			{
				continue;
			}

			if (!client->GetNetChan()) // Netchan valid?
			{
				continue;
			}

			std::string finalIpAddress = "null"; // If this stays null they modified the packet somehow.
			ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
			if (ipAddressField)
			{
				std::stringstream ss;
				ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
					<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

				finalIpAddress = ss.str();
			}

			if (onlyDigits)
			{
				std::int64_t ID = static_cast<std::int64_t>(std::stoll(firstArg));
				if (ID > MAX_PLAYERS) // Is it a possible originID?
				{
					std::int64_t originID = client->m_iOriginID;
					if (originID != ID) // See if they match.
					{
						continue;
					}
				}
				else // If its not try by userID.
				{
					std::int64_t clientID = static_cast<std::int64_t>(client->m_iUserID + 1); // Get UserID + 1.
					if (clientID != ID) // See if they match.
					{
						continue;
					}
				}

				g_pBanSystem->AddEntry(finalIpAddress, client->m_iOriginID); // Add ban entry.
				g_pBanSystem->Save(); // Save ban list.
				NET_DisconnectClient(client, i, "Banned from Server", 0, 1); // Disconnect client.
			}
			else
			{
				if (firstArg.compare(finalIpAddress) != NULL) // Do the string equal?
				{
					continue;
				}

				g_pBanSystem->AddEntry(finalIpAddress, client->m_iOriginID); // Add ban entry.
				g_pBanSystem->Save(); // Save ban list.
				NET_DisconnectClient(client, i, "Banned from Server", 0, 1); // Disconnect client.
			}
		}
	}
	catch (std::exception& e)
	{
		DevMsg(eDLL_T::SERVER, "Banid Error: %s", e.what());
		return;
	}
}

/*
=====================
_Unban_f_CompletionFunc
=====================
*/
void _Unban_f_CompletionFunc(CCommand* cmd)
{
	static auto HasOnlyDigits = [](const std::string& string)
	{
		for (const char& character : string)
		{
			if (std::isdigit(character) == 0)
			{
				return false;
			}
		}
		return true;
	};

	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.

	try
	{
		const char* firstArg = args[1];
		if (HasOnlyDigits(firstArg)) // Check if we have an ip address or origin ID.
		{
			g_pBanSystem->DeleteEntry("noIP", std::stoll(firstArg)); // Delete ban entry.
			g_pBanSystem->Save(); // Save modified vector to file.
		}
		else
		{
			g_pBanSystem->DeleteEntry(firstArg, 1); // Delete ban entry.
			g_pBanSystem->Save(); // Save modified vector to file.
		}
	}
	catch (std::exception& e)
	{
		DevMsg(eDLL_T::SERVER, "Unban Error: %s", e.what());
		return;
	}
}

/*
=====================
_ReloadBanList_f_CompletionFunc
=====================
*/
void _ReloadBanList_f_CompletionFunc(CCommand* cmd)
{
	g_pBanSystem->Load(); // Reload banlist.
}

/*
=====================
_RTech_StringToGUID_f_CompletionFunc
=====================
*/
void _RTech_StringToGUID_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);

	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	const char* firstArg = args[1]; // Get first arg.
	unsigned long long guid = g_pRtech->StringToGuid(firstArg);

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "] RTECH_HASH -------------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, "] GUID: '0x%llX'\n", guid);
}

/*
=====================
_RTech_AsyncLoad_f_CompletionFunc
=====================
*/
void _RTech_AsyncLoad_f_CompletionFunc(CCommand* cmd)
{
	CCommand& args = *cmd; // Get reference.
	std::string firstArg = args[1]; // Get first arg.

	HRtech_AsyncLoad(firstArg);
}

/*
=====================
_RTech_Decompress_f_CompletionFunc

  Decompresses input RPak file and
  dumps results to 'paks\Win32\*.rpak'
=====================
*/
void _RTech_Decompress_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);

	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	std::string firstArg  = args[1]; // Get first arg.
	std::string secondArg = args[2]; // Get second arg.

	const std::string modDir = "paks\\Win32\\";
	const std::string baseDir = "paks\\Win64\\";

	std::string pakNameOut = modDir + firstArg + ".rpak";
	std::string pakNameIn = baseDir + firstArg + ".rpak";

	CreateDirectories(pakNameOut);

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "] RTECH_DECOMPRESS -------------------------------------------\n");

	if (!FileExists(pakNameIn.c_str()))
	{
		DevMsg(eDLL_T::RTECH, "Error: pak file '%s' does not exist!\n", pakNameIn.c_str());
		return;
	}

	DevMsg(eDLL_T::RTECH, "] Processing: '%s'\n", pakNameIn.c_str());

	std::vector<std::uint8_t> upak; // Compressed region.
	std::ifstream ipak(pakNameIn, std::fstream::binary);

	ipak.seekg(0, std::fstream::end);
	upak.resize(ipak.tellg());
	ipak.seekg(0, std::fstream::beg);
	ipak.read((char*)upak.data(), upak.size());

	RPakHeader_t* rheader = (RPakHeader_t*)upak.data();
	uint16_t flags = (rheader->m_nFlags[0] << 8) | rheader->m_nFlags[1];

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "] HEADER_DETAILS ---------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, "] Magic    : '%08X'\n", rheader->m_nMagic);
	DevMsg(eDLL_T::RTECH, "] Version  : '%u'\n", (rheader->m_nVersion));
	DevMsg(eDLL_T::RTECH, "] Flags    : '%04X'\n", (flags));
	DevMsg(eDLL_T::RTECH, "] Hash     : '%llu'\n", rheader->m_nHash);
	DevMsg(eDLL_T::RTECH, "] Entries  : '%zu'\n", rheader->m_nAssetEntryCount);
	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "] COMPRESSION_DETAILS ----------------------------------------\n");
	DevMsg(eDLL_T::RTECH, "] Size disk: '%lld'\n", rheader->m_nSizeDisk);
	DevMsg(eDLL_T::RTECH, "] Size decp: '%lld'\n", rheader->m_nSizeMemory);
	DevMsg(eDLL_T::RTECH, "] Ratio    : '%.02f'\n", (rheader->m_nSizeDisk * 100.f) / rheader->m_nSizeMemory);

	if (rheader->m_nMagic != 'kaPR')
	{
		DevMsg(eDLL_T::RTECH, "Error: pak file '%s' has invalid magic!\n", pakNameIn.c_str());
		return;
	}
	if ((rheader->m_nFlags[1] & 1) != 1)
	{
		DevMsg(eDLL_T::RTECH, "Error: pak file '%s' already decompressed!\n", pakNameIn.c_str());
		return;
	}
	if (rheader->m_nSizeDisk != upak.size())
	{
		DevMsg(eDLL_T::RTECH, "Error: pak file '%s' decompressed size '%u' doesn't match expected value '%u'!\n", pakNameIn.c_str(), upak.size(), rheader->m_nSizeMemory);
		return;
	}

	RPakDecompState_t state;
	std::uint32_t decompSize = g_pRtech->DecompressPakFileInit(&state, upak.data(), upak.size(), 0, PAK_HEADER_SIZE);

	if (decompSize == rheader->m_nSizeDisk)
	{
		DevMsg(eDLL_T::RTECH, "Error: calculated size: '%zu' expected: '%zu'!\n", decompSize, rheader->m_nSizeMemory);
		return;
	}
	else
	{
		DevMsg(eDLL_T::RTECH, "] Calculated size: '%zu'\n", decompSize);
	}

	std::vector<std::uint8_t> pakBuf(rheader->m_nSizeMemory, 0);

	state.m_nOutMask = UINT64_MAX;
	state.m_nOut = uint64_t(pakBuf.data());

	std::uint8_t decompResult = g_pRtech->DecompressPakFile(&state, upak.size(), pakBuf.size());
	if (decompResult != 1)
	{
		DevMsg(eDLL_T::RTECH, "Error: decompression failed for '%s' return value: '%u'!\n", pakNameIn.c_str(), +decompResult);
		return;
	}

	rheader->m_nFlags[1] = 0x0; // Set compressed flag to false for the decompressed pak file
	rheader->m_nSizeDisk = rheader->m_nSizeMemory; // Equal compressed size with decompressed

	std::ofstream outBlock(pakNameOut, std::fstream::binary);

	if (rheader->m_nPatchIndex > 0) // Check if its an patch rpak.
	{
		// Loop through all the structs and patch their compress size.
		for (int i = 1, patch_offset = 0x88; i <= rheader->m_nPatchIndex; i++, patch_offset += sizeof(RPakPatchCompressedHeader_t))
		{
			RPakPatchCompressedHeader_t* patch_header = (RPakPatchCompressedHeader_t*)((std::uintptr_t)pakBuf.data() + patch_offset);
			patch_header->m_nSizeDisk = patch_header->m_nSizeMemory; // Fix size for decompress.
		}
	}

	memcpy_s(pakBuf.data(), state.m_nDecompSize, ((std::uint8_t*)rheader), PAK_HEADER_SIZE); // Overwrite first 0x80 bytes which are NULL with the header data.

	outBlock.write((char*)pakBuf.data(), state.m_nDecompSize);

	uint32_t crc32_init = {};
	DevMsg(eDLL_T::RTECH, "] CRC32          : '%08X'\n", crc32::update(crc32_init, pakBuf.data(), state.m_nDecompSize));
	DevMsg(eDLL_T::RTECH, "] Decompressed rpak to: '%s'\n", pakNameOut.c_str());
	DevMsg(eDLL_T::RTECH, "--------------------------------------------------------------\n");

	outBlock.close();
}

/*
=====================
_NET_TraceNetChan_f_CompletionFunc

  Logs all data transmitted and received
  over the UDP socket to a file on the disk.
  File: '<mod\logs\net_trace.log>'.
=====================
*/
void _NET_TraceNetChan_f_CompletionFunc(CCommand* cmd)
{
	static bool bTraceNetChannel = false;
	if (!bTraceNetChannel)
	{
		g_pCVar->FindVar("net_usesocketsforloopback")->SetValue(1);
		DevMsg(eDLL_T::ENGINE, "\n");
		DevMsg(eDLL_T::ENGINE, "+--------------------------------------------------------+\n");
		DevMsg(eDLL_T::ENGINE, "|>>>>>>>>>>>>>| NETCHANNEL TRACE ACTIVATED |<<<<<<<<<<<<<|\n");
		DevMsg(eDLL_T::ENGINE, "+--------------------------------------------------------+\n");
		DevMsg(eDLL_T::ENGINE, "\n");

		// Begin the detour transaction to hook the the process
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		CNetChan_Trace_Attach();
		// Commit the transaction
		if (DetourTransactionCommit() != NO_ERROR)
		{
			// Failed to hook into the process, terminate
			TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
		}
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "\n");
		DevMsg(eDLL_T::ENGINE, "+--------------------------------------------------------+\n");
		DevMsg(eDLL_T::ENGINE, "|>>>>>>>>>>>>| NETCHANNEL TRACE DEACTIVATED |<<<<<<<<<<<<|\n");
		DevMsg(eDLL_T::ENGINE, "+--------------------------------------------------------+\n");
		DevMsg(eDLL_T::ENGINE, "\n");

		// Begin the detour transaction to hook the the process
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		CNetChan_Trace_Detach();

		// Commit the transaction
		DetourTransactionCommit();
	}
	bTraceNetChannel = !bTraceNetChannel;
}

/*
=====================
_VPK_Decompress_f_CompletionFunc

  Decompresses input VPK files and
  dumps the output to '<mod>\vpk'.
=====================
*/
void _VPK_Decompress_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);

	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	std::string firstArg = args[1]; // Get first arg.
	std::string szPathOut = "platform\\vpk";

	std::chrono::milliseconds msStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	DevMsg(eDLL_T::FS, "______________________________________________________________\n");
	DevMsg(eDLL_T::FS, "] FS_DECOMPRESS ----------------------------------------------\n");
	DevMsg(eDLL_T::FS, "] Processing: '%s'\n", firstArg.c_str());

	vpk_dir_h vpk = g_pPackedStore->GetPackDirFile(firstArg);
	g_pPackedStore->InitLzDecompParams();

	std::thread th([&] { g_pPackedStore->UnpackAll(vpk, szPathOut); });
	th.join();

	std::chrono::milliseconds msEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	float duration = msEnd.count() - msStart.count();

	DevMsg(eDLL_T::FS, "______________________________________________________________\n");
	DevMsg(eDLL_T::FS, "] OPERATION_DETAILS ------------------------------------------\n");
	DevMsg(eDLL_T::FS, "] Time elapsed: '%.3f' seconds\n", (duration / 1000));
	DevMsg(eDLL_T::FS, "] Decompressed vpk to: '%s'\n", szPathOut.c_str());
	DevMsg(eDLL_T::FS, "--------------------------------------------------------------\n");
}

/*
=====================
_NET_SetKey_f_CompletionFunc

  Sets the input netchannel encryption key
=====================
*/
void _NET_SetKey_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);

	if (argSize < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	CCommand& args = *cmd; // Get reference.
	std::string firstArg = args[1]; // Get first arg.

	HNET_SetKey(firstArg);
}

/*
=====================
_NET_GenerateKey_f_CompletionFunc

  Sets a random netchannel encryption key
=====================
*/
void _NET_GenerateKey_f_CompletionFunc(CCommand* cmd)
{
	HNET_GenerateKey();
}
#ifndef DEDICATED
/*
=====================
_RCON_CmdQuery_f_CompletionFunc

  Issues an RCON command to the
  RCON server.
=====================
*/
void _RCON_CmdQuery_f_CompletionFunc(CCommand* cmd)
{
	std::int32_t argSize = *(std::int32_t*)((std::uintptr_t)cmd + 0x4);
	CCommand& args = *cmd; // Get reference.

	switch (argSize)
	{
		case 0:
		case 1:
		{
			if (g_pRConClient->IsInitialized()
				&& !g_pRConClient->IsConnected()
				&& strlen(rcon_address->GetString()) > 0)
			{
				g_pRConClient->Connect();
			}
			break;
		}
		case 2:
		{
			if (!g_pRConClient->IsInitialized())
			{
				DevMsg(eDLL_T::CLIENT, "Failed to issue command to RCON server: uninitialized\n");
				break;
			}
			else if (g_pRConClient->IsConnected())
			{
				if (strcmp(args[1], "PASS") == 0) // Auth with RCON server using rcon_password ConVar value.
				{
					std::string svCmdQuery = g_pRConClient->Serialize(args[1], rcon_password->GetString(), cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
					g_pRConClient->Send(svCmdQuery);
					break;
				}
				else if (strcmp(args[1], "disconnect") == 0) // Disconnect from RCON server.
				{
					g_pRConClient->Disconnect();
					break;
				}

				std::string svCmdQuery = g_pRConClient->Serialize(args[1], "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
				g_pRConClient->Send(svCmdQuery);
				break;
			}
			else
			{
				DevMsg(eDLL_T::CLIENT, "Failed to issue command to RCON server: unconnected\n");
				break;
			}
			break;
		}
		case 3:
		{
			if (g_pRConClient->IsConnected())
			{
				if (strcmp(args[1], "PASS") == 0) // Auth with RCON server.
				{
					std::string svCmdQuery = g_pRConClient->Serialize(args[1], args[2], cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
					g_pRConClient->Send(svCmdQuery);
					break;
				}

				std::string svCmdQuery = g_pRConClient->Serialize(args[1], args[2], cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE);
				g_pRConClient->Send(svCmdQuery);
				break;
			}
			else
			{
				DevMsg(eDLL_T::CLIENT, "Failed to issue command to RCON server: unconnected\n");
				break;
			}
			break;
		}
	}
}
#endif // !DEDICATED
