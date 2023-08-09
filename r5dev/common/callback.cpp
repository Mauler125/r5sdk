//=============================================================================//
//
// Purpose: Callback functions for ConVar's.
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/init.h"
#include "windows/id3dx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "engine/client/cl_rcon.h"
#include "engine/client/cdll_engine_int.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#include "engine/net.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/enginetrace.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "vpklib/packedstore.h"
#include "vscript/vscript.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#endif // !CLIENT_DLL
#include "public/edict.h"
#include "public/worldsize.h"
#include "mathlib/crc32.h"
#include "mathlib/mathlib.h"
#include "common/completion.h"
#include "common/callback.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#endif // !DEDICATED
#include "public/bspflags.h"
#include "public/cmodel.h"
#include "public/idebugoverlay.h"
#ifndef CLIENT_DLL
#include "game/server/detour_impl.h"
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/cliententitylist.h"
#include "game/client/viewrender.h"
#endif // !DEDICATED


/*
=====================
MP_GameMode_Changed_f
=====================
*/
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	SetupGamemode(mp_gamemode->GetString());
}

/*
=====================
MP_HostName_Changed_f
=====================
*/
void MP_HostName_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
#ifndef DEDICATED
	g_pBrowser->SetHostName(pylon_matchmaking_hostname->GetString());
#endif // !DEDICATED
}

#ifndef DEDICATED
/*
=====================
ToggleConsole_f
=====================
*/
void ToggleConsole_f(const CCommand& args)
{
	g_pConsole->m_bActivate ^= true;
	ResetInput(); // Disable input to game when console is drawn.
}

/*
=====================
ToggleBrowser_f
=====================
*/
void ToggleBrowser_f(const CCommand& args)
{
	g_pBrowser->m_bActivate ^= true;
	ResetInput(); // Disable input to game when browser is drawn.
}
#endif // !DEDICATED
#ifndef CLIENT_DLL
/*
=====================
Host_Kick_f

  helper function for
  bansystem
=====================
*/
void _Author_Client_f(const CCommand& args, EKickType type)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	const char* szReason = args.ArgC() > 2 ? args.Arg(2) : nullptr;

	switch(type)
	{
		case KICK_NAME:
		{
			g_pBanSystem->KickPlayerByName(args.Arg(1), szReason);
			break;
		}
		case KICK_ID:
		{
			g_pBanSystem->KickPlayerById(args.Arg(1), szReason);
			break;
		}
		case BAN_NAME:
		{
			g_pBanSystem->BanPlayerByName(args.Arg(1), szReason);
			break;
		}
		case BAN_ID:
		{
			g_pBanSystem->BanPlayerById(args.Arg(1), szReason);
			break;
		}
		default:
		{
			// Code bug.
			Assert(0);
		}
	}
}


/*
=====================
Host_Kick_f
=====================
*/
void Host_Kick_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::KICK_NAME);
}

/*
=====================
Host_KickID_f
=====================
*/
void Host_KickID_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::KICK_ID);
}

/*
=====================
Host_Ban_f
=====================
*/
void Host_Ban_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::BAN_NAME);
}

/*
=====================
Host_BanID_f
=====================
*/
void Host_BanID_f(const CCommand& args)
{
	_Author_Client_f(args, EKickType::BAN_ID);
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

	g_pBanSystem->UnbanPlayer(args.Arg(1));
}

/*
=====================
Host_ReloadBanList_f
=====================
*/
void Host_ReloadBanList_f(const CCommand& args)
{
	g_pBanSystem->Load(); // Reload banned list.
}

/*
=====================
Host_ReloadPlaylists_f
=====================
*/
void Host_ReloadPlaylists_f(const CCommand& args)
{
	_DownloadPlaylists_f();
	KeyValues::InitPlaylists(); // Re-Init playlist.
}

/*
=====================
Host_Changelevel_f

  Goes to a new map, 
  taking all clients along
=====================
*/
void Host_Changelevel_f(const CCommand& args)
{
	if (args.ArgC() >= 2
		&& IsOriginInitialized()
		&& g_pServer->IsActive())
	{
		v_SetLaunchOptions(args);
		v_HostState_ChangeLevelMP(args[1], args[2]);
	}
}

/*
=====================
Detour_HotSwap_f

  Hot swaps the NavMesh
  while the game is running
=====================
*/
void Detour_HotSwap_f(const CCommand& args)
{
	if (!g_pServer->IsActive())
		return; // Only execute if server is initialized and active.

	DevMsg(eDLL_T::SERVER, "Executing NavMesh hot swap for level '%s'\n",
		g_ServerGlobalVariables->m_pszMapName);

	CFastTimer timer;

	timer.Start();
	Detour_HotSwap();

	timer.End();
	DevMsg(eDLL_T::SERVER, "Hot swap took '%lf' seconds\n", timer.GetDuration().GetSeconds());
}
#endif // !CLIENT_DLL
/*
=====================
Pak_ListPaks_f
=====================
*/
void Pak_ListPaks_f(const CCommand& args)
{
	DevMsg(eDLL_T::RTECH, "| id   | name                                               | status                               | asset count |\n");
	DevMsg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");

	uint32_t nTotalLoaded = 0;

	for (int16_t i = 0, n = *g_pLoadedPakCount; i < n; ++i)
	{
		const RPakLoadedInfo_t& info = g_pLoadedPakInfo[i];

		if (info.m_nStatus == RPakStatus_t::PAK_STATUS_FREED)
			continue;

		const char* szRpakStatus = g_pRTech->PakStatusToString(info.m_nStatus);

		// todo: make status into a string from an array/vector
		DevMsg(eDLL_T::RTECH, "| %04i | %-50s | %-36s | %11i |\n", info.m_nHandle, info.m_pszFileName, szRpakStatus, info.m_nAssetCount);
		nTotalLoaded++;
	}
	DevMsg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
	DevMsg(eDLL_T::RTECH, "| %18i loaded paks.                                                                                |\n", nTotalLoaded);
	DevMsg(eDLL_T::RTECH, "|------|----------------------------------------------------|--------------------------------------|-------------|\n");
}

/*
=====================
Pak_ListTypes_f
=====================
*/
void Pak_ListTypes_f(const CCommand& args)
{
	DevMsg(eDLL_T::RTECH, "| ext  | description               | version | header size | native size |\n");
	DevMsg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");

	uint32_t nRegistered = 0;

	for (int8_t i = 0; i < PAK_MAX_TYPES; ++i)
	{
		RPakAssetBinding_t* type = &g_pPakGlobals->m_nAssetBindings[i];

		if (!type->m_szDescription)
			continue;

		DevMsg(eDLL_T::RTECH, "| %-4s | %-25s | %7i | %11i | %11i |\n", FourCCToString(type->m_nExtension).c_str(), type->m_szDescription, type->m_iVersion, type->m_iSubHeaderSize, type->m_iNativeClassSize);
		nRegistered++;
	}
	DevMsg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");
	DevMsg(eDLL_T::RTECH, "| %18i registered types.                                   |\n", nRegistered);
	DevMsg(eDLL_T::RTECH, "|------|---------------------------|---------|-------------|-------------|\n");
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
			const RPakHandle_t pakHandle = atoi(args.Arg(1));
			const RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(pakHandle);
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified handle.");
			}

			const string pakName = pakInfo->m_pszFileName;
			!pakName.empty() ? DevMsg(eDLL_T::RTECH, "Requested pak unload for file '%s'\n", pakName.c_str()) : DevMsg(eDLL_T::RTECH, "Requested pak unload for handle '%d'\n", pakHandle);
			g_pakLoadApi->UnloadPak(pakHandle);
		}
		else
		{
			const RPakLoadedInfo_t* pakInfo = g_pRTech->GetPakLoadedInfo(args.Arg(1));
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified name.");
			}

			DevMsg(eDLL_T::RTECH, "Requested pak unload for file '%s'\n", args.Arg(1));
			g_pakLoadApi->UnloadPak(pakInfo->m_nHandle);
		}
	}
	catch (const std::exception& e)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - %s\n", __FUNCTION__, e.what());
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
		string pakName;
		RPakHandle_t pakHandle = 0;
		RPakLoadedInfo_t* pakInfo = nullptr;

		if (args.HasOnlyDigits(1))
		{
			pakHandle = atoi(args.Arg(1));
			pakInfo = g_pRTech->GetPakLoadedInfo(pakHandle);
			if (!pakInfo)
			{
				throw std::exception("Found no pak entry for specified handle.");
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

			pakHandle = pakInfo->m_nHandle;
		}

		!pakName.empty() ? DevMsg(eDLL_T::RTECH, "Requested pak swap for file '%s'\n", pakName.c_str()) : DevMsg(eDLL_T::RTECH, "Requested pak swap for handle '%d'\n", pakHandle);

		g_pakLoadApi->UnloadPak(pakHandle);

		while (pakInfo->m_nStatus != RPakStatus_t::PAK_STATUS_FREED) // Wait till this slot gets free'd.
			std::this_thread::sleep_for(std::chrono::seconds(1));

		g_pakLoadApi->LoadAsync(pakName.c_str());
	}
	catch (const std::exception& e)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - %s\n", __FUNCTION__, e.what());
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
	DevMsg(eDLL_T::RTECH, "] RTECH_HASH ]------------------------------------------------\n");
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

	CUtlString inPakFile;
	CUtlString outPakFile;

	inPakFile.Format(PLATFORM_PAK_PATH "%s", args.Arg(1));
	outPakFile.Format(PLATFORM_PAK_OVERRIDE_PATH "%s", args.Arg(1));

	DevMsg(eDLL_T::RTECH, "______________________________________________________________\n");
	DevMsg(eDLL_T::RTECH, "-+ RTech decompress ------------------------------------------\n");

	if (!FileSystem()->FileExists(inPakFile.String(), "GAME"))
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - pak file '%s' does not exist!\n",
			__FUNCTION__, inPakFile.String());
		return;
	}

	DevMsg(eDLL_T::RTECH, " |-+ Processing: '%s'\n", inPakFile.String());
	FileHandle_t hPakFile = FileSystem()->Open(inPakFile.String(), "rb", "GAME");

	if (!hPakFile)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n",
			__FUNCTION__, inPakFile.String());
		return;
	}

	const ssize_t nPakLen = FileSystem()->Size(hPakFile);

	std::unique_ptr<uint8_t[]> pPakBufContainer(new uint8_t[nPakLen]);
	uint8_t* const pPakBuf = pPakBufContainer.get();

	FileSystem()->Read(pPakBuf, nPakLen, hPakFile);
	FileSystem()->Close(hPakFile);

	RPakHeader_t* pHeader = reinterpret_cast<RPakHeader_t*>(pPakBuf);
	uint16_t flags = (pHeader->m_nFlags[0] << 8) | pHeader->m_nFlags[1];

	SYSTEMTIME systemTime;
	FileTimeToSystemTime(&pHeader->m_nFileTime, &systemTime);

	DevMsg(eDLL_T::RTECH, " | |-+ Header ------------------------------------------------\n");
	DevMsg(eDLL_T::RTECH, " |   |-- Magic    : '0x%08X'\n", pHeader->m_nMagic);
	DevMsg(eDLL_T::RTECH, " |   |-- Version  : '%hu'\n", pHeader->m_nVersion);
	DevMsg(eDLL_T::RTECH, " |   |-- Flags    : '0x%04hX'\n", flags);
	DevMsg(eDLL_T::RTECH, " |   |-- Time     : '%hu-%hu-%hu/%hu %hu:%hu:%hu.%hu'\n",
		systemTime.wYear,systemTime.wMonth,systemTime.wDay, systemTime.wDayOfWeek,
		systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
	DevMsg(eDLL_T::RTECH, " |   |-- Hash     : '0x%08llX'\n", pHeader->m_nHash);
	DevMsg(eDLL_T::RTECH, " |   |-- Entries  : '%u'\n", pHeader->m_nAssetEntryCount);
	DevMsg(eDLL_T::RTECH, " |   |-+ Compression -----------------------------------------\n");
	DevMsg(eDLL_T::RTECH, " |     |-- Size comp: '%zu'\n", pHeader->m_nSizeDisk);
	DevMsg(eDLL_T::RTECH, " |     |-- Size decp: '%zu'\n", pHeader->m_nSizeMemory);

	if (pHeader->m_nMagic != PAK_HEADER_MAGIC)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - pak file '%s' has invalid magic!\n",
			__FUNCTION__, inPakFile.String());

		return;
	}
	if ((pHeader->m_nFlags[1] & 1) != 1)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - pak file '%s' already decompressed!\n",
			__FUNCTION__, inPakFile.String());

		return;
	}

	const size_t unsignedPakLen = static_cast<size_t>(nPakLen);

	if (pHeader->m_nSizeDisk != unsignedPakLen)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - pak file '%s' decompressed size '%zu' doesn't match expected size '%zu'!\n",
			__FUNCTION__, inPakFile.String(), unsignedPakLen, pHeader->m_nSizeMemory);

		return;
	}

	RPakDecompState_t decompState;
	const uint64_t nDecompSize = g_pRTech->DecompressPakFileInit(&decompState, pPakBuf, unsignedPakLen, NULL, sizeof(RPakHeader_t));

	if (nDecompSize == pHeader->m_nSizeDisk)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - calculated size: '%llu' expected: '%llu'!\n",
			__FUNCTION__, nDecompSize, pHeader->m_nSizeMemory);

		return;
	}
	else
	{
		DevMsg(eDLL_T::RTECH, " |     |-- Size calc: '%llu'\n", nDecompSize);
	}

	DevMsg(eDLL_T::RTECH, " |     |-- Ratio    : '%.02f'\n", (pHeader->m_nSizeDisk * 100.f) / pHeader->m_nSizeMemory);


	std::unique_ptr<uint8_t[]> pDecompBufContainer(new uint8_t[nPakLen]);
	uint8_t* const pDecompBuf = pDecompBufContainer.get();

	decompState.m_nOutMask = UINT64_MAX;
	decompState.m_nOut = uint64_t(pDecompBuf);

	uint8_t nDecompResult = g_pRTech->DecompressPakFile(&decompState, unsignedPakLen, pHeader->m_nSizeMemory);
	if (nDecompResult != 1)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - decompression failed for '%s' return value: '%hu'!\n",
			__FUNCTION__, inPakFile.String(), nDecompResult);
	}

	pHeader->m_nFlags[1] = 0x0; // Set compressed flag to false for the decompressed pak file.
	pHeader->m_nSizeDisk = pHeader->m_nSizeMemory; // Equal compressed size with decompressed.

	FileSystem()->CreateDirHierarchy(PLATFORM_PAK_OVERRIDE_PATH, "GAME");
	FileHandle_t hDecompFile = FileSystem()->Open(outPakFile.String(), "wb", "GAME");

	if (!hDecompFile)
	{
		Error(eDLL_T::RTECH, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n",
			__FUNCTION__, outPakFile.String());

		return;
	}

	if (pHeader->m_nPatchIndex > 0) // Check if its an patch rpak.
	{
		// Loop through all the structs and patch their compress size.
		for (uint32_t i = 1, nPatchOffset = (sizeof(RPakHeader_t) + sizeof(uint64_t));
			i <= pHeader->m_nPatchIndex; i++, nPatchOffset += sizeof(RPakPatchCompressedHeader_t))
		{
			RPakPatchCompressedHeader_t* pPatchHeader = reinterpret_cast<RPakPatchCompressedHeader_t*>(pDecompBuf + nPatchOffset);
			DevMsg(eDLL_T::RTECH, " |     |-+ Patch #%02u -----------------------------------------\n", i);
			DevMsg(eDLL_T::RTECH, " |     %s |-- Size comp: '%llu'\n", i < pHeader->m_nPatchIndex ? "|" : " ", pPatchHeader->m_nSizeDisk);
			DevMsg(eDLL_T::RTECH, " |     %s |-- Size decp: '%llu'\n", i < pHeader->m_nPatchIndex ? "|" : " ", pPatchHeader->m_nSizeMemory);

			pPatchHeader->m_nSizeDisk = pPatchHeader->m_nSizeMemory; // Fix size for decompress.
		}
	}

	memcpy_s(pDecompBuf, sizeof(RPakHeader_t), pPakBuf, sizeof(RPakHeader_t));// Overwrite first 0x80 bytes which are NULL with the header data.
	FileSystem()->Write(pDecompBuf, decompState.m_nDecompSize, hDecompFile);

	DevMsg(eDLL_T::RTECH, " |-- Checksum : '0x%08X'\n", crc32::update(NULL, pDecompBuf, decompState.m_nDecompSize));
	DevMsg(eDLL_T::RTECH, "-+ Decompressed pak file to: '%s'\n", outPakFile.String());
	DevMsg(eDLL_T::RTECH, "--------------------------------------------------------------\n");

	FileSystem()->Close(hDecompFile);
}

/*
=====================
VPK_Pack_f

  Packs VPK files into
  'PLATFORM' VPK directory.
=====================
*/
void VPK_Pack_f(const CCommand& args)
{
	if (args.ArgC() < 4)
	{
		return;
	}

	VPKPair_t pair(args.Arg(1), args.Arg(2), args.Arg(3), NULL);
	CFastTimer timer;

	DevMsg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", pair.m_DirName.Get());
	timer.Start();

	g_pPackedStore->InitLzCompParams();
	g_pPackedStore->PackWorkspace(pair, fs_packedstore_workspace->GetString(), "vpk/");

	timer.End();
	DevMsg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
	DevMsg(eDLL_T::FS, "\n");
}

/*
=====================
VPK_Unpack_f

  Unpacks VPK files into
  workspace directory.
=====================
*/
void VPK_Unpack_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	CUtlString arg = args.Arg(1);
	VPKDir_t vpk(arg, (args.ArgC() > 2));
	CFastTimer timer;

	DevMsg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", arg.Get());
	timer.Start();

	g_pPackedStore->InitLzDecompParams();
	g_pPackedStore->UnpackWorkspace(vpk, fs_packedstore_workspace->GetString());

	timer.End();
	DevMsg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
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

	FileSystem()->MountVPKFile(args.Arg(1));
}

/*
=====================
VPK_Unmount_f

  Unmounts input VPK file
  and clears its cache
=====================
*/
void VPK_Unmount_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	FileSystem()->UnmountVPKFile(args.Arg(1));
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

/*
=====================
NET_UseRandomKeyChanged_f

  Use random AES encryption
  key for game packets
=====================
*/
void NET_UseRandomKeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same value.

		if (pConVarRef->GetBool())
			NET_GenerateKey();
		else
			NET_SetKey(DEFAULT_NET_ENCRYPTION_KEY);
	}
}

/*
=====================
NET_UseSocketsForLoopbackChanged_f

  Use random AES encryption
  key for game packets
=====================
*/
void NET_UseSocketsForLoopbackChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same value.

#ifndef CLIENT_DLL
		// Reboot the RCON server to switch address type.
		if (RCONServer()->IsInitialized())
		{
			DevMsg(eDLL_T::SERVER, "Rebooting RCON server...\n");
			RCONServer()->Shutdown();
			RCONServer()->Init();
		}
#endif // !CLIENT_DLL
	}
}

/*
=====================
SIG_GetAdr_f

  Logs the sigscan
  results to the console.
=====================
*/
void SIG_GetAdr_f(const CCommand& args)
{
	DetourAddress();
}

/*
=====================
CON_Help_f

  Shows the colors and
  description of each
  context.
=====================
*/
void CON_Help_f(const CCommand& args)
{
	DevMsg(eDLL_T::COMMON, "Contexts:\n");
	SQVM_PrintFunc(reinterpret_cast<HSQUIRRELVM>(SQCONTEXT::SERVER), (SQChar*)(" = Server DLL (Script)\n"));
	SQVM_PrintFunc(reinterpret_cast<HSQUIRRELVM>(SQCONTEXT::CLIENT), (SQChar*)(" = Client DLL (Script)\n"));
	SQVM_PrintFunc(reinterpret_cast<HSQUIRRELVM>(SQCONTEXT::UI), (SQChar*)(" = UI DLL (Script)\n"));

	DevMsg(eDLL_T::SERVER, " = Server DLL (Code)\n");
	DevMsg(eDLL_T::CLIENT, " = Client DLL (Code)\n");
	DevMsg(eDLL_T::UI, " = UI DLL (Code)\n");
	DevMsg(eDLL_T::ENGINE, " = Engine DLL (Code)\n");
	DevMsg(eDLL_T::FS, " = FileSystem (Code)\n");
	DevMsg(eDLL_T::RTECH, " = PakLoad API (Code)\n");
	DevMsg(eDLL_T::MS, " = MaterialSystem (Code)\n");
	DevMsg(eDLL_T::AUDIO, " = Audio DLL (Code)\n");
	DevMsg(eDLL_T::VIDEO, " = Video DLL (Code)\n");
	DevMsg(eDLL_T::NETCON, " = NetConsole (Code)\n");
}

#ifndef DEDICATED
/*
=====================
CON_LogHistory_f

  Shows the game console 
  submission history.
=====================
*/
void CON_LogHistory_f(const CCommand& args)
{
	const vector<string> vHistory = g_pConsole->GetHistory();
	for (size_t i = 0, nh = vHistory.size(); i < nh; i++)
	{
		DevMsg(eDLL_T::COMMON, "%3d: %s\n", i, vHistory[i].c_str());
	}
}

/*
=====================
CON_RemoveLine_f

  Removes a range of lines
  from the console.
=====================
*/
void CON_RemoveLine_f(const CCommand& args)
{
	if (args.ArgC() < 3)
	{
		DevMsg(eDLL_T::CLIENT, "Usage 'con_removeline': start(int) end(int)\n");
		return;
	}

	int start = atoi(args[1]);
	int end = atoi(args[2]);

	g_pConsole->RemoveLog(start, end);
}

/*
=====================
CON_ClearLines_f

  Clears all lines from
  the developer console.
=====================
*/
void CON_ClearLines_f(const CCommand& args)
{
	g_pConsole->ClearLog();
}

/*
=====================
CON_ClearHistory_f

  Clears all submissions from the
  developer console history.
=====================
*/
void CON_ClearHistory_f(const CCommand& args)
{
	g_pConsole->ClearHistory();
}

/*
=====================
RCON_CmdQuery_f

  Issues an RCON command to the
  RCON server.
=====================
*/
void RCON_CmdQuery_f(const CCommand& args)
{
	const int64_t argCount = args.ArgC();

	if (argCount < 2)
	{
		const char* pszAddress = rcon_address->GetString();

		if (RCONClient()->IsInitialized()
			&& !RCONClient()->IsConnected()
			&& pszAddress[0])
		{
			RCONClient()->Connect(pszAddress);
		}
	}
	else
	{
		if (!RCONClient()->IsInitialized())
		{
			Warning(eDLL_T::CLIENT, "Failed to issue command to RCON server: %s\n", "uninitialized");
			return;
		}
		else if (RCONClient()->IsConnected())
		{
			vector<char> vecMsg;
			bool bSuccess = false;
			const SocketHandle_t hSocket = RCONClient()->GetSocket();

			if (strcmp(args.Arg(1), "PASS") == 0) // Auth with RCON server using rcon_password ConVar value.
			{
				if (argCount > 2)
				{
					bSuccess = RCONClient()->Serialize(vecMsg, args.Arg(2), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				else // Use 'rcon_password' ConVar as password.
				{
					bSuccess = RCONClient()->Serialize(vecMsg, rcon_password->GetString(), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}

				if (bSuccess)
				{
					RCONClient()->Send(hSocket, vecMsg.data(), int(vecMsg.size()));
				}

				return;
			}
			else if (strcmp(args.Arg(1), "disconnect") == 0) // Disconnect from RCON server.
			{
				RCONClient()->Disconnect("issued by user");
				return;
			}

			bSuccess = RCONClient()->Serialize(vecMsg, args.Arg(1), args.ArgS(), cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND);
			if (bSuccess)
			{
				RCONClient()->Send(hSocket, vecMsg.data(), int(vecMsg.size()));
			}
			return;
		}
		else
		{
			Warning(eDLL_T::CLIENT, "Failed to issue command to RCON server: %s\n", "unconnected");
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
	const bool bIsConnected = RCONClient()->IsConnected();
	RCONClient()->Disconnect("issued by user");

	if (bIsConnected) // Log if client was indeed connected.
	{
		DevMsg(eDLL_T::CLIENT, "User closed RCON connection\n");
	}
}

/*
=====================
RCON_SendLogs_f

  request logs from RCON server
=====================
*/
void RCON_InputOnlyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	RCONClient()->RequestConsoleLog(RCONClient()->ShouldReceive());
}
#endif // !DEDICATED

#ifndef CLIENT_DLL

static const char* s_LanguageNames[] = {
	"english",
	"french",
	"german",
	"italian",
	"japanese",
	"polish",
	"russian",
	"spanish",
	"schinese",
	"tchinese",
	"korean"
};

static bool IsValidTextLanguage(const char* pLocaleName)
{
	for (int i = 0; i < SDK_ARRAYSIZE(s_LanguageNames); ++i)
	{
		if (strcmp(pLocaleName, s_LanguageNames[i]) == NULL)
			return true;
	}

	return false;
}

void SV_LanguageChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		const char* pNewString = pConVarRef->GetString();

		if (IsValidTextLanguage(pNewString))
			return;

		// if new text isn't valid but the old value is, reset the value
		if (IsValidTextLanguage(pOldString))
		{
			pConVarRef->SetValue(pOldString);
			return;
		}
		else // this shouldn't really happen, but if neither the old nor new values are valid, set to english
			pConVarRef->SetValue("english");

	}
}

#endif

/*
=====================
RCON_PasswordChanged_f

  Change RCON password
  on server and client
=====================
*/
void RCON_PasswordChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same password.

#ifndef DEDICATED
		if (!RCONClient()->IsInitialized())
			RCONClient()->Init(); // Initialize first.
#endif // !DEDICATED
#ifndef CLIENT_DLL
		if (RCONServer()->IsInitialized())
			RCONServer()->SetPassword(pConVarRef->GetString());
		else
			RCONServer()->Init(); // Initialize first.
#endif // !CLIENT_DLL
	}
}

#ifndef CLIENT_DLL
/*
=====================
RCON_WhiteListAddresChanged_f

  Change whitelist address
  on RCON server
=====================
*/
void RCON_WhiteListAddresChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same address.

		if (!RCONServer()->SetWhiteListAddress(pConVarRef->GetString()))
		{
			Warning(eDLL_T::SERVER, "Failed to set RCON whitelist address: %s\n", pConVarRef->GetString());
		}
	}
}

/*
=====================
RCON_ConnectionCountChanged_f

  Change max connection
  count on RCON server
=====================
*/
void RCON_ConnectionCountChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue)
{
	if (!RCONServer()->IsInitialized())
		return; // Not initialized; no sockets at this point.

	if (ConVar* pConVarRef = g_pCVar->FindVar(pConVar->GetCommandName()))
	{
		if (strcmp(pOldString, pConVarRef->GetString()) == NULL)
			return; // Same count.

		const int maxCount = pConVarRef->GetInt();

		int count = RCONServer()->GetAuthenticatedCount();
		CSocketCreator* pCreator = RCONServer()->GetSocketCreator();

		if (count < maxCount)
		{
			if (!pCreator->IsListening())
			{
				pCreator->CreateListenSocket(*RCONServer()->GetNetAddress());
			}
		}
		else
		{
			while (count > maxCount)
			{
				RCONServer()->Disconnect(count-1, "too many authenticated sockets");
				count = RCONServer()->GetAuthenticatedCount();
			}

			pCreator->CloseListenSocket();
			RCONServer()->CloseNonAuthConnection();
		}
	}
}
#endif // !CLIENT_DLL

/*
=====================
SQVM_ServerScript_f

  Executes input on the
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

  Executes input on the
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

  Executes input on the
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
		DevMsg(eDLL_T::MS, " |-- Streaming texture count: '%d'\n", material->m_nStreamableTextureCount);
		DevMsg(eDLL_T::MS, " |-- Material width: '%d'\n", material->m_iWidth);
		DevMsg(eDLL_T::MS, " |-- Material height: '%d'\n", material->m_iHeight);
		DevMsg(eDLL_T::MS, " |-- Flags: '%llX'\n", material->m_iFlags);

		std::function<void(CMaterialGlue*, const char*)> fnPrintChild = [](CMaterialGlue* material, const char* print)
		{
			DevMsg(eDLL_T::MS, " |-+\n");
			DevMsg(eDLL_T::MS, " | |-+ Child material ----------------------------------------\n");
			DevMsg(eDLL_T::MS, print, material);
			DevMsg(eDLL_T::MS, " |     |-- GUID: '%llX'\n", material->m_GUID);
			DevMsg(eDLL_T::MS, " |     |-- Material name: '%s'\n", material->m_pszName);
		};

		DevMsg(eDLL_T::MS, " |-- Material name: '%s'\n", material->m_pszName);
		DevMsg(eDLL_T::MS, " |-- Material surface name 1: '%s'\n", material->m_pszSurfaceProp);
		DevMsg(eDLL_T::MS, " |-- Material surface name 2: '%s'\n", material->m_pszSurfaceProp2);
		DevMsg(eDLL_T::MS, " |-- DX buffer: '%llX'\n", material->m_pDXBuffer);
		DevMsg(eDLL_T::MS, " |-- DX buffer VFTable: '%llX'\n", material->m_pID3D11BufferVTable);

		material->m_pDepthShadow 
			? fnPrintChild(material->m_pDepthShadow, " |   |-+ DepthShadow: '%llX'\n") 
			: DevMsg(eDLL_T::MS, " |   |-+ DepthShadow: 'NULL'\n");
		material->m_pDepthPrepass 
			? fnPrintChild(material->m_pDepthPrepass, " |   |-+ DepthPrepass: '%llX'\n") 
			: DevMsg(eDLL_T::MS, " |   |-+ DepthPrepass: 'NULL'\n");
		material->m_pDepthVSM 
			? fnPrintChild(material->m_pDepthVSM, " |   |-+ DepthVSM: '%llX'\n") 
			: DevMsg(eDLL_T::MS, " |   |-+ DepthVSM: 'NULL'\n");
		material->m_pDepthShadow 
			? fnPrintChild(material->m_pDepthShadow, " |   |-+ DepthShadowTight: '%llX'\n") 
			: DevMsg(eDLL_T::MS, " |   |-+ DepthShadowTight: 'NULL'\n");
		material->m_pColPass 
			? fnPrintChild(material->m_pColPass, " |   |-+ ColPass: '%llX'\n") 
			: DevMsg(eDLL_T::MS, " |   |-+ ColPass: 'NULL'\n");

		DevMsg(eDLL_T::MS, "-+ Texture GUID map ------------------------------------------\n");
		DevMsg(eDLL_T::MS, " |-- Texture handles: '%llX'\n", material->m_pTextureHandles);
		DevMsg(eDLL_T::MS, " |-- Streaming texture handles: '%llX'\n", material->m_pStreamableTextureHandles);

		DevMsg(eDLL_T::MS, "--------------------------------------------------------------\n");
	}
	else
	{
		DevMsg(eDLL_T::MS, "%s: No material found >:(\n", __FUNCTION__);
	}
}

/*
=====================
Line_f

  Draws a line at 
  start<x1 y1 z1> end<x2 y2 z2>.
=====================
*/
void Line_f(const CCommand& args)
{
	if (args.ArgC() != 7)
	{
		DevMsg(eDLL_T::CLIENT, "Usage 'line': start(vector) end(vector)\n");
		return;
	}

	Vector3D start, end;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
		end[i] = float(atof(args[i + 4]));
	}

	g_pDebugOverlay->AddLineOverlay(start, end, 255, 255, 0, !r_debug_draw_depth_test->GetBool(), 100);
}

/*
=====================
Sphere_f

  Draws a sphere at origin(x1 y1 z1) 
  radius(float) theta(int) phi(int).
=====================
*/
void Sphere_f(const CCommand& args)
{
	if (args.ArgC() != 7)
	{
		DevMsg(eDLL_T::CLIENT, "Usage 'sphere': origin(vector) radius(float) theta(int) phi(int)\n");
		return;
	}

	Vector3D start;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
	}

	float radius = float(atof(args[4]));
	int theta = atoi(args[5]);
	int phi = atoi(args[6]);

	g_pDebugOverlay->AddSphereOverlay(start, radius, theta, phi, 20, 210, 255, 0, 100);
}

/*
=====================
Capsule_f

  Draws a capsule at start<x1 y1 z1> 
  end<x2 y2 z2> radius <x3 y3 z3>.
=====================
*/
void Capsule_f(const CCommand& args)
{
	if (args.ArgC() != 10)
	{
		DevMsg(eDLL_T::CLIENT, "Usage 'capsule': start(vector) end(vector) radius(vector)\n");
		return;
	}

	Vector3D start, end, radius;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
		end[i] = float(atof(args[i + 4]));
		radius[i] = float(atof(args[i + 7]));
	}
	g_pDebugOverlay->AddCapsuleOverlay(start, end, radius, { 0,0,0 }, { 0,0,0 }, 141, 233, 135, 0, 100);
}
#endif // !DEDICATED
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
/*
=====================
BHit_f

  Bullet trajectory tracing
  from shooter to target entity.
=====================
*/
void BHit_f(const CCommand& args)
{
#ifndef CLIENT_DLL // Stubbed to suppress server warnings as this is a GAMEDLL command!
	if (args.ArgC() != 9)
		return;

	if (!bhit_enable->GetBool())
		return;

	if (sv_visualizetraces->GetBool())
	{
		Vector3D vecAbsStart;
		Vector3D vecAbsEnd;

		for (int i = 0; i < 3; ++i)
			vecAbsStart[i] = float(atof(args[i + 4]));

		QAngle vecBulletAngles;
		for (int i = 0; i < 2; ++i)
			vecBulletAngles[i] = float(atof(args[i + 7]));

		vecBulletAngles.z = 180.f; // Flipped axis.
		AngleVectors(vecBulletAngles, &vecAbsEnd);

		vecAbsEnd.MulAdd(vecAbsStart, vecAbsEnd, MAX_COORD_RANGE);

		Ray_t ray(vecAbsStart, vecAbsEnd);
		trace_t trace;

		g_pEngineTraceServer->TraceRay(ray, TRACE_MASK_NPCWORLDSTATIC, &trace);

		g_pDebugOverlay->AddLineOverlay(trace.startpos, trace.endpos, 0, 255, 0, !bhit_depth_test->GetBool(), sv_visualizetraces_duration->GetFloat());
		g_pDebugOverlay->AddLineOverlay(trace.endpos, vecAbsEnd, 255, 0, 0, !bhit_depth_test->GetBool(), sv_visualizetraces_duration->GetFloat());
	}
#endif // !CLIENT_DLL

#ifndef DEDICATED
	if (bhit_abs_origin->GetBool() && r_visualizetraces->GetBool())
	{
		const int iEnt = atoi(args[2]);
		if (const IClientEntity* pEntity = g_pClientEntityList->GetClientEntity(iEnt))
		{
			g_pDebugOverlay->AddSphereOverlay( // Render a debug sphere at the client's predicted entity origin.
				pEntity->GetAbsOrigin(), 10.f, 8, 6, 20, 60, 255, 0, r_visualizetraces_duration->GetFloat());
		}
	}
#endif // !DEDICATED
}
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
/*
=====================
CVHelp_f

  Show help text for a
  particular convar/concommand
=====================
*/
void CVHelp_f(const CCommand& args)
{
	cv->CvarHelp(args);
}

/*
=====================
CVList_f

  List all ConCommandBases
=====================
*/
void CVList_f(const CCommand& args)
{
	cv->CvarList(args);
}

/*
=====================
CVDiff_f

  List all ConVar's 
  who's values deviate 
  from default value
=====================
*/
void CVDiff_f(const CCommand& args)
{
	cv->CvarDifferences(args);
}

/*
=====================
CVFlag_f

  List all ConVar's
  with specified flags
=====================
*/
void CVFlag_f(const CCommand& args)
{
	cv->CvarFindFlags_f(args);
}

/*
=====================
CC_CreateFakePlayer_f

  Creates a fake player
  on the server
=====================
*/
#ifndef CLIENT_DLL
void CC_CreateFakePlayer_f(const CCommand& args)
{
	if (args.ArgC() < 3)
	{
		DevMsg(eDLL_T::SERVER, "usage 'sv_addbot': name(string) teamid(int)\n");
		return;
	}

	int numPlayers = g_pServer->GetNumClients();

	// Already at max, don't create.
	if (numPlayers >= g_ServerGlobalVariables->m_nMaxClients)
		return;

	const char* playerName = args.Arg(1);

	int teamNum = atoi(args.Arg(2));
	int maxTeams = int(g_pServer->GetMaxTeams()) + 1;

	// Clamp team count, going above the limit will
	// cause a crash. Going below 0 means that the
	// engine will assign the bot to the last team.
	if (teamNum > maxTeams)
		teamNum = maxTeams;

	g_pEngineServer->LockNetworkStringTables(true);

	edict_t nHandle = g_pEngineServer->CreateFakeClient(playerName, teamNum);
	g_pServerGameClients->ClientFullyConnect(nHandle, false);

	g_pEngineServer->LockNetworkStringTables(false);
}
#endif // !CLIENT_DLL