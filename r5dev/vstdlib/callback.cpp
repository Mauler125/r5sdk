//=============================================================================//
//
// Purpose: Callback functions for ConVar's.
//
//=============================================================================//

#include "core/stdafx.h"
#include "windows/id3dx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#ifndef DEDICATED
#include "engine/client/cl_rcon.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#include "engine/net.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "vpklib/packedstore.h"
#include "squirrel/sqscript.h"
#ifndef DEDICATED
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#include "public/include/bansystem.h"
#include "mathlib/crc32.h"
#include "vstdlib/completion.h"
#include "vstdlib/callback.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#endif // !DEDICATED


/*
=====================
MP_GameMode_Changed_f
=====================
*/
bool MP_GameMode_Changed_f(ConVar* pVTable)
{
	return SetupGamemode(mp_gamemode->GetString());
}

#ifndef DEDICATED
/*
=====================
GameConsole_Invoke_f
=====================
*/
void GameConsole_Invoke_f(const CCommand& args)
{
	g_pConsole->m_bActivate = !g_pConsole->m_bActivate;
}

/*
=====================
ServerBrowser_Invoke_f
=====================
*/
void ServerBrowser_Invoke_f(const CCommand& args)
{
	g_pBrowser->m_bActivate = !g_pBrowser->m_bActivate;
}
#endif // !DEDICATED

/*
=====================
Host_Kick_f
=====================
*/
void Host_Kick_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		string svClientName = pNetChan->GetName(); // Get full name.

		if (svClientName.empty())
		{
			continue;
		}

		if (strcmp(args.Arg(1), svClientName.c_str()) != 0) // Our wanted name?
		{
			continue;
		}

		NET_DisconnectClient(pClient, i, "Kicked from server", 0, 1);
	}
}

/*
=====================
Host_KickID_f
=====================
*/
void Host_KickID_f(const CCommand& args)
{
	if (args.ArgC() < 2) // Do we atleast have 2 arguments?
	{
		return;
	}

	try
	{
		bool bOnlyDigits = args.HasOnlyDigits(1);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CClient* pClient = g_pClient->GetClient(i);
			if (!pClient)
				continue;

			CNetChan* pNetChan = pClient->GetNetChan();
			if (!pNetChan)
				continue;

			string svIpAddress = pNetChan->GetAddress(); // If this stays null they modified the packet somehow.

			if (bOnlyDigits)
			{
				uint64_t nTargetID = static_cast<uint64_t>(std::stoll(args.Arg(1)));
				if (nTargetID > MAX_PLAYERS) // Is it a possible originID?
				{
					uint64_t nOriginID = pClient->GetOriginID();
					if (nOriginID != nTargetID)
					{
						continue;
					}
				}
				else // If its not try by handle.
				{
					uint64_t nClientID = static_cast<uint64_t>(pClient->GetHandle());
					if (nClientID != nTargetID)
					{
						continue;
					}
				}

				NET_DisconnectClient(pClient, i, "Kicked from server", 0, 1);
			}
			else
			{
				if (string(args.Arg(1)).compare(svIpAddress) != NULL)
				{
					continue;
				}

				NET_DisconnectClient(pClient, i, "Kicked from server", 0, 1);
			}
		}
	}
	catch (std::exception& e)
	{
		Error(eDLL_T::SERVER, "%s - sv_kickid requires a UserID or OriginID. You can get the UserID with the 'status' command. Error: %s", __FUNCTION__, e.what());
		return;
	}
}

/*
=====================
Host_Ban_f
=====================
*/
void Host_Ban_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		CNetChan* pNetChan = pClient->GetNetChan();
		if (!pNetChan)
			continue;

		string svClientName = pNetChan->GetName(); // Get full name.

		if (svClientName.empty())
		{
			continue;
		}

		if (strcmp(args.Arg(1), svClientName.c_str()) != 0)
		{
			continue;
		}

		string svIpAddress = pNetChan->GetAddress(); // If this stays empty they modified the packet somehow.

		g_pBanSystem->AddEntry(svIpAddress, pClient->GetOriginID());
		g_pBanSystem->Save();
		NET_DisconnectClient(pClient, i, "Banned from server", 0, 1);
	}
}

/*
=====================
Host_BanID_f
=====================
*/
void Host_BanID_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	try
	{
		bool bOnlyDigits = args.HasOnlyDigits(1);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CClient* pClient = g_pClient->GetClient(i);
			if (!pClient)
				continue;

			CNetChan* pNetChan = pClient->GetNetChan();
			if (!pNetChan)
				continue;

			string svIpAddress = pNetChan->GetAddress(); // If this stays empty they modified the packet somehow.

			if (bOnlyDigits)
			{
				uint64_t nTargetID = static_cast<uint64_t>(std::stoll(args.Arg(1)));
				if (nTargetID > static_cast<uint64_t>(MAX_PLAYERS)) // Is it a possible originID?
				{
					uint64_t nOriginID = pClient->GetOriginID();
					if (nOriginID != nTargetID)
					{
						continue;
					}
				}
				else // If its not try by handle.
				{
					uint64_t nClientID = static_cast<uint64_t>(pClient->GetHandle());
					if (nClientID != nTargetID)
					{
						continue;
					}
				}

				g_pBanSystem->AddEntry(svIpAddress, pClient->GetOriginID());
				g_pBanSystem->Save();
				NET_DisconnectClient(pClient, i, "Banned from server", 0, 1);
			}
			else
			{
				if (string(args.Arg(1)).compare(svIpAddress) != NULL)
				{
					continue;
				}

				g_pBanSystem->AddEntry(svIpAddress, pClient->GetOriginID());
				g_pBanSystem->Save();
				NET_DisconnectClient(pClient, i, "Banned from server", 0, 1);
			}
		}
	}
	catch (std::exception& e)
	{
		Error(eDLL_T::SERVER, "%s - Banid Error: %s", __FUNCTION__, e.what());
		return;
	}
}

/*
=====================
Host_Unban_f
=====================
*/
void Host_Unban_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	try
	{
		if (args.HasOnlyDigits(1)) // Check if we have an ip address or origin ID.
		{
			g_pBanSystem->DeleteEntry("noIP", std::stoll(args.Arg(1))); // Delete ban entry.
			g_pBanSystem->Save(); // Save modified vector to file.
		}
		else
		{
			g_pBanSystem->DeleteEntry(args.Arg(1), 0); // Delete ban entry.
			g_pBanSystem->Save(); // Save modified vector to file.
		}
	}
	catch (std::exception& e)
	{
		Error(eDLL_T::SERVER, "%s - Unban error: %s", __FUNCTION__, e.what());
		return;
	}
}

/*
=====================
Host_ReloadBanList_f
=====================
*/
void Host_ReloadBanList_f(const CCommand& args)
{
	g_pBanSystem->Load(); // Reload banlist.
}

/*
=====================
Pak_ListPaks_f
=====================
*/
void Pak_ListPaks_f(const CCommand& args)
{
	DevMsg(eDLL_T::RTECH, "| id | name                                               | status                               | asset count |\n");
	DevMsg(eDLL_T::RTECH, "|----|----------------------------------------------------|--------------------------------------|-------------|\n");

	uint32_t nActuallyLoaded = 0;

	for (int16_t i = 0; i < *s_pLoadedPakCount; ++i)
	{
		RPakLoadedInfo_t info = g_pLoadedPakInfo[i];

		if (info.m_nStatus == RPakStatus_t::PAK_STATUS_FREED)
			continue;

		string rpakStatus = "RPAK_CREATED_A_NEW_STATUS_SOMEHOW";

		auto it = RPakStatusToString.find(info.m_nStatus);
		if (it != RPakStatusToString.end())
			rpakStatus = it->second;

		// todo: make status into a string from an array/vector
		DevMsg(eDLL_T::RTECH, "| %02i | %-50s | %-36s | %11i |\n", info.m_nPakId, info.m_pszFileName, rpakStatus.c_str(), info.m_nAssetCount);
		nActuallyLoaded++;
	}
	DevMsg(eDLL_T::RTECH, "|----|----------------------------------------------------|--------------------------------------|-------------|\n");
	DevMsg(eDLL_T::RTECH, "| %16i loaded paks.                                                                                |\n", nActuallyLoaded);
	DevMsg(eDLL_T::RTECH, "|----|----------------------------------------------------|--------------------------------------|-------------|\n");
}

/*
=====================
Pak_RequestUnload_f
=====================
*/
void Pak_RequestUnload_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	try
	{
		if (args.HasOnlyDigits(1))
		{
			RPakHandle_t nPakId = std::stoi(args.Arg(1));
			RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(nPakId);
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified ID.");
			}

			string pakName = pakInfo->m_pszFileName;
			!pakName.empty() ? DevMsg(eDLL_T::RTECH, "Requested pak unload for '%s'\n", pakName.c_str()) : DevMsg(eDLL_T::RTECH, "Requested Pak Unload for '%d'\n", nPakId);
			g_pakLoadApi->UnloadPak(nPakId);
		}
		else
		{
			RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(args.Arg(1));
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified name.");
			}

			DevMsg(eDLL_T::RTECH, "Requested pak unload for '%s'\n", args.Arg(1));
			g_pakLoadApi->UnloadPak(pakInfo->m_nPakId);
		}
	}
	catch (std::exception& e)
	{
		Error(eDLL_T::RTECH, "%s - %s", __FUNCTION__, e.what());
		return;
	}
}

/*
=====================
Pak_RequestLoad_f
=====================
*/
void Pak_RequestLoad_f(const CCommand& args)
{
	g_pakLoadApi->LoadAsync(args.Arg(1));
}


/*
=====================
Pak_Swap_f
=====================
*/
void Pak_Swap_f(const CCommand& args)
{
	try
	{
		RPakHandle_t nPakId = 0;
		RPakLoadedInfo_t* pakInfo = nullptr;
		string pakName = std::string();

		if (args.HasOnlyDigits(1))
		{
			nPakId = std::stoi(args.Arg(1));
			pakInfo = g_pRTech->GetPakLoadedInfo(nPakId);
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified ID.");
			}

			pakName = pakInfo->m_pszFileName;
		}
		else
		{
			pakName = args.Arg(1);
			pakInfo = g_pRTech->GetPakLoadedInfo(args.Arg(1));
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified name.");
			}

			nPakId = pakInfo->m_nPakId;
		}

		!pakName.empty() ? DevMsg(eDLL_T::RTECH, "Requested pak swap for '%s'\n", pakName.c_str()) : DevMsg(eDLL_T::RTECH, "Requested pak swap for '%d'\n", nPakId);

		g_pakLoadApi->UnloadPak(nPakId);

		while (pakInfo->m_nStatus != RPakStatus_t::PAK_STATUS_FREED) // Wait till this slot gets free'd.
			std::this_thread::sleep_for(std::chrono::seconds(1));

		g_pakLoadApi->LoadAsync(pakName.c_str());
	}
	catch (std::exception& e)
	{
		Error(eDLL_T::RTECH, "%s - %s", __FUNCTION__, e.what());
		return;
	}
}

/*
=====================
RTech_StringToGUID_f
=====================
*/
void RTech_StringToGUID_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	unsigned long long guid = g_pRTech->StringToGuid(args.Arg(1));

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "] RTECH_HASH -------------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, "] GUID: '0x%llX'\n", guid);
}

/*
=====================
RTech_Decompress_f

  Decompresses input RPak file and
  dumps results to 'paks\Win32\*.rpak'
=====================
*/
void RTech_Decompress_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const string modDir = "paks\\Win32\\";
	const string baseDir = "paks\\Win64\\";

	string pakNameOut = modDir + args.Arg(1);
	string pakNameIn = baseDir + args.Arg(1);

	CreateDirectories(pakNameOut);

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "-+ RTech decompress ------------------------------------------\n");

	if (!FileExists(pakNameIn))
	{
		Error(eDLL_T::RTECH, "%s - pak file '%s' does not exist!\n", __FUNCTION__, pakNameIn.c_str());
		return;
	}

	DevMsg(eDLL_T::RTECH, " |-+ Processing: '%s'\n", pakNameIn.c_str());

	CIOStream reader(pakNameIn, CIOStream::Mode_t::READ);

	RPakHeader_t rheader = reader.Read<RPakHeader_t>();
	uint16_t flags = (rheader.m_nFlags[0] << 8) | rheader.m_nFlags[1];

	DevMsg(eDLL_T::RTECH, " | |-+ Header ------------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, " | | |-- Magic    : '%08X'\n", rheader.m_nMagic);
	DevMsg(eDLL_T::RTECH, " | | |-- Version  : '%hu'\n", rheader.m_nVersion);
	DevMsg(eDLL_T::RTECH, " | | |-- Flags    : '%04hX'\n", flags);
	DevMsg(eDLL_T::RTECH, " | | |-- Hash     : '%llu%llu'\n", rheader.m_nHash0, rheader.m_nHash1);
	DevMsg(eDLL_T::RTECH, " | | |-- Entries  : '%u'\n", rheader.m_nAssetEntryCount);
	DevMsg(eDLL_T::RTECH, " | |-+ Compression -------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, " | | |-- Size disk: '%llu'\n", rheader.m_nSizeDisk);
	DevMsg(eDLL_T::RTECH, " | | |-- Size decp: '%llu'\n", rheader.m_nSizeMemory);
	DevMsg(eDLL_T::RTECH, " | | |-- Ratio    : '%.02f'\n", (rheader.m_nSizeDisk * 100.f) / rheader.m_nSizeMemory);

	if (rheader.m_nMagic != 'kaPR')
	{
		Error(eDLL_T::RTECH, "%s - pak file '%s' has invalid magic!\n", __FUNCTION__, pakNameIn.c_str());
		return;
	}
	if ((rheader.m_nFlags[1] & 1) != 1)
	{
		Error(eDLL_T::RTECH, "%s - pak file '%s' already decompressed!\n", __FUNCTION__, pakNameIn.c_str());
		return;
	}
	if (rheader.m_nSizeDisk != reader.GetSize())
	{
		Error(eDLL_T::RTECH, "%s - pak file '%s' decompressed size '%zu' doesn't match expected value '%llu'!\n", __FUNCTION__, pakNameIn.c_str(), reader.GetSize(), rheader.m_nSizeMemory);
		return;
	}

	RPakDecompState_t state;
	uint64_t decompSize = g_pRTech->DecompressPakFileInit(&state, const_cast<uint8_t*>(reader.GetData()), reader.GetSize(), NULL, sizeof(RPakHeader_t));

	if (decompSize == rheader.m_nSizeDisk)
	{
		Error(eDLL_T::RTECH, "%s - calculated size: '%llu' expected: '%llu'!\n", __FUNCTION__, decompSize, rheader.m_nSizeMemory);
		return;
	}
	else
	{
		DevMsg(eDLL_T::RTECH, " | | |-- Calculated size: '%llu'\n", decompSize);
	}

	vector<uint8_t> pakBuf(rheader.m_nSizeMemory, 0);

	state.m_nOutMask = UINT64_MAX;
	state.m_nOut = uint64_t(pakBuf.data());

	uint8_t decompResult = g_pRTech->DecompressPakFile(&state, reader.GetSize(), pakBuf.size());
	if (decompResult != 1)
	{
		Error(eDLL_T::RTECH, "%s - decompression failed for '%s' return value: '%hu'!\n", __FUNCTION__, pakNameIn.c_str(), decompResult);
		return;
	}

	rheader.m_nFlags[1] = 0x0; // Set compressed flag to false for the decompressed pak file.
	rheader.m_nSizeDisk = rheader.m_nSizeMemory; // Equal compressed size with decompressed.

	CIOStream writer(pakNameOut, CIOStream::Mode_t::WRITE);

	if (rheader.m_nPatchIndex > 0) // Check if its an patch rpak.
	{
		// Loop through all the structs and patch their compress size.
		for (uint32_t i = 1, patch_offset = 0x88; i <= rheader.m_nPatchIndex; i++, patch_offset += sizeof(RPakPatchCompressedHeader_t))
		{
			RPakPatchCompressedHeader_t* patch_header = (RPakPatchCompressedHeader_t*)((uintptr_t)pakBuf.data() + patch_offset);
			patch_header->m_nSizeDisk = patch_header->m_nSizeMemory; // Fix size for decompress.
		}
	}

	memcpy_s(pakBuf.data(), state.m_nDecompSize, &rheader, sizeof(RPakHeader_t)); // Overwrite first 0x80 bytes which are NULL with the header data.
	writer.Write(pakBuf.data(), state.m_nDecompSize);

	DevMsg(eDLL_T::RTECH, " | | |-- CRC32          : '%08X'\n", crc32::update(NULL, pakBuf.data(), state.m_nDecompSize));
	DevMsg(eDLL_T::RTECH, " |-+ Decompressed rpak to: '%s'\n", pakNameOut.c_str());
	DevMsg(eDLL_T::RTECH, "--------------------------------------------------------------\n");
}

/*
=====================
VPK_Pack_f

  Compresses new VPK files and
  dumps the output to '\vpk'.
=====================
*/
void VPK_Pack_f(const CCommand& args)
{
	if (args.ArgC() < 4)
	{
		return;
	}
	std::chrono::milliseconds msStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	g_pPackedStore->InitLzCompParams();
	VPKPair_t vPair = g_pPackedStore->BuildFileName(args.Arg(1), args.Arg(2), args.Arg(3), NULL);

	DevMsg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", vPair.m_svDirectoryName.c_str());

	std::thread th([&] { g_pPackedStore->PackAll(vPair, fs_packedstore_workspace->GetString(), "vpk/", (args.ArgC() > 4)); });
	th.join();

	std::chrono::milliseconds msEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	float duration = msEnd.count() - msStart.count();

	DevMsg(eDLL_T::FS, "*** Time elapsed: '%.3f' seconds\n", (duration / 1000));
	DevMsg(eDLL_T::FS, "\n");
}

/*
=====================
VPK_Unpack_f

  Decompresses input VPK files and
  dumps the output to '<mod>\vpk'.
=====================
*/
void VPK_Unpack_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}
	std::chrono::milliseconds msStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

	DevMsg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", args.Arg(1));

	VPKDir_t vpk = g_pPackedStore->GetDirectoryFile(args.Arg(1));
	g_pPackedStore->InitLzDecompParams();

	std::thread th([&] { g_pPackedStore->UnpackAll(vpk, ConvertToWinPath(fs_packedstore_workspace->GetString())); });
	th.join();

	std::chrono::milliseconds msEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	float duration = msEnd.count() - msStart.count();

	DevMsg(eDLL_T::FS, "*** Time elapsed: '%.3f' seconds\n", (duration / 1000));
	DevMsg(eDLL_T::FS, "\n");
}

/*
=====================
VPK_Mount_f

  Mounts input VPK file for
  internal FileSystem usage
=====================
*/
void VPK_Mount_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	VPKData_t* pPakData = FileSystem()->MountVPK(args.Arg(1));
	if (pPakData)
	{
		DevMsg(eDLL_T::FS, "Mounted VPK file '%s' with handle '%i'\n", args.Arg(1), pPakData->m_nHandle);
	}
	else
	{
		Warning(eDLL_T::FS, "Unable to mount VPK file '%s': non-existent VPK file\n", args.Arg(1));
	}
}

/*
=====================
NET_SetKey_f

  Sets the input netchannel encryption key
=====================
*/
void NET_SetKey_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	NET_SetKey(args.Arg(1));
}

/*
=====================
NET_GenerateKey_f

  Sets a random netchannel encryption key
=====================
*/
void NET_GenerateKey_f(const CCommand& args)
{
	NET_GenerateKey();
}
#ifndef DEDICATED
/*
=====================
RCON_CmdQuery_f

  Issues an RCON command to the
  RCON server.
=====================
*/
void RCON_CmdQuery_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		if (g_pRConClient->IsInitialized()
			&& !g_pRConClient->IsConnected()
			&& strlen(rcon_address->GetString()) > 0)
		{
			g_pRConClient->Connect();
		}
	}
	else
	{
		if (!g_pRConClient->IsInitialized())
		{
			Warning(eDLL_T::CLIENT, "Failed to issue command to RCON server: uninitialized\n");
			return;
		}
		else if (g_pRConClient->IsConnected())
		{
			if (strcmp(args.Arg(1), "PASS") == 0) // Auth with RCON server using rcon_password ConVar value.
			{
				string svCmdQuery;
				if (args.ArgC() > 2)
				{
					svCmdQuery = g_pRConClient->Serialize(args.Arg(2), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				else // Use 'rcon_password' ConVar as password.
				{
					svCmdQuery = g_pRConClient->Serialize(rcon_password->GetString(), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				g_pRConClient->Send(svCmdQuery);
				return;
			}
			else if (strcmp(args.Arg(1), "disconnect") == 0) // Disconnect from RCON server.
			{
				g_pRConClient->Disconnect();
				return;
			}

			string svCmdQuery = g_pRConClient->Serialize(args.ArgS(), "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
			g_pRConClient->Send(svCmdQuery);
			return;
		}
		else
		{
			Warning(eDLL_T::CLIENT, "Failed to issue command to RCON server: unconnected\n");
			return;
		}
	}
}

/*
=====================
RCON_Disconnect_f

  Disconnect from RCON server
=====================
*/
void RCON_Disconnect_f(const CCommand& args)
{
	if (g_pRConClient->IsConnected())
	{
		g_pRConClient->Disconnect();
		DevMsg(eDLL_T::CLIENT, "User closed RCON connection\n");
	}
}
#endif // !DEDICATED

/*
=====================
SQVM_ServerScript_f

  Exectutes input on the
  VM in SERVER context.
=====================
*/
void SQVM_ServerScript_f(const CCommand& args)
{
	if (args.ArgC() >= 2)
	{
		Script_Execute(args.ArgS(), SQCONTEXT::SERVER);
	}
}

#ifndef DEDICATED
/*
=====================
SQVM_ClientScript_f

  Exectutes input on the
  VM in CLIENT context.
=====================
*/
void SQVM_ClientScript_f(const CCommand& args)
{
	if (args.ArgC() >= 2)
	{
		Script_Execute(args.ArgS(), SQCONTEXT::CLIENT);
	}
}

/*
=====================
SQVM_UIScript_f

  Exectutes input on the
  VM in UI context.
=====================
*/
void SQVM_UIScript_f(const CCommand& args)
{
	if (args.ArgC() >= 2)
	{
		Script_Execute(args.ArgS(), SQCONTEXT::UI);
	}
}

/*
=====================
Mat_CrossHair_f

  Print the material under the crosshair.
=====================
*/
void Mat_CrossHair_f(const CCommand& args)
{
	CMaterialGlue* material = GetMaterialAtCrossHair();
	if (material)
	{
		DevMsg(eDLL_T::MS, "______________________________________________________________\n");
		DevMsg(eDLL_T::MS, "-+ Material --------------------------------------------------\n");
		DevMsg(eDLL_T::MS, " |-- ADDR: '%llX'\n", material);
		DevMsg(eDLL_T::MS, " |-- GUID: '%llX'\n", material->m_GUID);
		DevMsg(eDLL_T::MS, " |-- Signature: '%d'\n", material->m_UnknownSignature);
		DevMsg(eDLL_T::MS, " |-- Material Width: '%d'\n", material->m_iWidth);
		DevMsg(eDLL_T::MS, " |-- Material Height: '%d'\n", material->m_iHeight);
		DevMsg(eDLL_T::MS, " |-- Flags: '%llX'\n", material->m_iFlags);

		std::function<void(CMaterialGlue*, const char*)> fnPrintChild = [](CMaterialGlue* material, const char* print)
		{
			DevMsg(eDLL_T::MS, " |-+\n");
			DevMsg(eDLL_T::MS, " | |-+ Child material ----------------------------------------\n");
			DevMsg(eDLL_T::MS, print, material);
			DevMsg(eDLL_T::MS, " |     |-- GUID: '%llX'\n", material->m_GUID);
			DevMsg(eDLL_T::MS, " |     |-- Material Name: '%s'\n", material->m_pszName);
		};

		DevMsg(eDLL_T::MS, " |-- Material Name: '%s'\n", material->m_pszName);
		DevMsg(eDLL_T::MS, " |-- Material Surface Name 1: '%s'\n", material->m_pszSurfaceName1);
		DevMsg(eDLL_T::MS, " |-- Material Surface Name 2: '%s'\n", material->m_pszSurfaceName2);
		DevMsg(eDLL_T::MS, " |-- DX Texture 1: '%llX'\n", material->m_ppDXTexture1);
		DevMsg(eDLL_T::MS, " |-- DX Texture 2: '%llX'\n", material->m_ppDXTexture2);

		material->m_pDepthShadow ? fnPrintChild(material->m_pDepthShadow, " |   |-+ DepthShadow Addr: '%llX'\n") : DevMsg(eDLL_T::MS, " |   |-+ DepthShadow Addr: 'NULL'\n");
		material->m_pDepthPrepass ? fnPrintChild(material->m_pDepthPrepass, " |   |-+ DepthPrepass Addr: '%llX'\n") : DevMsg(eDLL_T::MS, " |   |-+ DepthPrepass Addr: 'NULL'\n");
		material->m_pDepthVSM ? fnPrintChild(material->m_pDepthVSM, " |   |-+ DepthVSM Addr: '%llX'\n") : DevMsg(eDLL_T::MS, " |   |-+ DepthVSM Addr: 'NULL'\n");
		material->m_pDepthShadow ? fnPrintChild(material->m_pDepthShadow, " |   |-+ DepthShadowTight Addr: '%llX'\n") : DevMsg(eDLL_T::MS, " |   |-+ DepthShadowTight Addr: 'NULL'\n");
		material->m_pColPass ? fnPrintChild(material->m_pColPass, " |   |-+ ColPass Addr: '%llX'\n") : DevMsg(eDLL_T::MS, " |   |-+ ColPass Addr: 'NULL'\n");

		DevMsg(eDLL_T::MS, "-+ Texture GUID map ------------------------------------------\n");
		material->m_pTextureGUID1 ? DevMsg(eDLL_T::MS, " |-- TextureMap 1 Addr: '%llX'\n", material->m_pTextureGUID1) : DevMsg(eDLL_T::MS, " |-- TextureMap 1 Addr: 'NULL'\n");
		material->m_pTextureGUID2 ? DevMsg(eDLL_T::MS, " |-- TextureMap 2 Addr: '%llX'\n", material->m_pTextureGUID2) : DevMsg(eDLL_T::MS, " |-- TextureMap 2 Addr: 'NULL'\n");

		DevMsg(eDLL_T::MS, "--------------------------------------------------------------\n");
	}
	else
	{
		DevMsg(eDLL_T::MS, "%s - No Material found >:(\n", __FUNCTION__);
	}
}
#endif // !DEDICATED