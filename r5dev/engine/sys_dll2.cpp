//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/strtools.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/traceinit.h"
#include "rtech/rtech_utils.h"
#ifndef DEDICATED
#include "client/vengineclient_impl.h"
#endif // !DEDICATED
#include "filesystem/filesystem.h"

//-----------------------------------------------------------------------------
// Figure out if we're running a Valve mod or not.
//-----------------------------------------------------------------------------
static bool IsValveMod(const char* pModName)
{
	return (Q_stricmp(pModName, "cstrike") == 0 ||
		Q_stricmp(pModName, "dod") == 0 ||
		Q_stricmp(pModName, "hl1mp") == 0 ||
		Q_stricmp(pModName, "tf") == 0 ||
		Q_stricmp(pModName, "hl2mp") == 0 ||
		Q_stricmp(pModName, "csgo") == 0);
}

//-----------------------------------------------------------------------------
// Figure out if we're running a Respawn mod or not.
//-----------------------------------------------------------------------------
static bool IsRespawnMod(const char* pModName)
{
	return (Q_stricmp(pModName, "r1") == 0 ||
		Q_stricmp(pModName, "r2") == 0 ||
		Q_stricmp(pModName, "r5") == 0);
}

//-----------------------------------------------------------------------------
// Initialize the VPK and file cache system
//-----------------------------------------------------------------------------
static void InitVPKSystem()
{
    char szCacheEnableFilePath[280]; // [rsp+20h] [rbp-118h] BYREF
    char bFixSlashes = FileSystem()->GetCurrentDirectory(szCacheEnableFilePath, 260) ? szCacheEnableFilePath[0] : '\0';

    size_t nCachePathLen = strlen(szCacheEnableFilePath);
    size_t nCacheFileLen = 15; // sizeof '/vpk/enable.txt' - 1;

    if ((nCachePathLen + nCacheFileLen) < 0x104 || (nCacheFileLen = 259 - nCachePathLen, nCachePathLen != 259))
    {
        strncat(szCacheEnableFilePath, "/vpk/enable.txt", nCacheFileLen)[259] = '\0';
        bFixSlashes = szCacheEnableFilePath[0];
    }
    if (bFixSlashes)
    {
        V_FixSlashes(szCacheEnableFilePath, '/');
    }
    if (!CommandLine()->CheckParm("-novpk") && FileSystem()->FileExists(szCacheEnableFilePath, nullptr))
    {
        FileSystem()->AddSearchPath(".", "MAIN", SearchPathAdd_t::PATH_ADD_TO_TAIL);
#ifndef DEDICATED
        FileSystem()->SetVPKCacheModeClient();
        FileSystem()->MountVPKFile("vpk/client_frontend.bsp");
#else // Dedicated runs server vpk's and must have 'vpk/mp_lobby.bsp' mounted.
        FileSystem()->SetVPKCacheModeServer();
        FileSystem()->MountVPKFile("vpk/server_mp_lobby.bsp");
#endif // !DEDICATED
    }
}

//-----------------------------------------------------------------------------
// Initialization, shutdown of a mod.
//-----------------------------------------------------------------------------
bool CEngineAPI::VModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)
{
	ConCommand::InitShipped();
	ConCommand::PurgeShipped();
	ConVar::InitShipped();
	ConVar::PurgeShipped();

    // Register new Pak Assets here!
    //RTech_RegisterAsset(0, 1, "", nullptr, nullptr, nullptr, CMemory(0x1660AD0A8).RCast<void**>(), 8, 8, 8, 0, 0xFFFFFFC);

	bool results = CEngineAPI_ModInit(pEngineAPI, pModName, pGameDir);
	if (!IsValveMod(pModName) && !IsRespawnMod(pModName))
	{
#ifndef DEDICATED
		g_pEngineClient->SetRestrictServerCommands(true); // Restrict server commands.
		g_pEngineClient->SetRestrictClientCommands(true); // Restrict client commands.
#endif // !DEDICATED
	}

	return results;
}

//-----------------------------------------------------------------------------
// Sets startup info
//-----------------------------------------------------------------------------
void CEngineAPI::VSetStartupInfo(CEngineAPI* pEngineAPI, StartupInfo_t* pStartupInfo)
{
    if (*g_bTextMode)
    {
        return;
    }

    strncpy(&*g_szBaseDir, pStartupInfo->m_szBaseDirectory, 260);

    g_pEngineParms->baseDirectory = &*g_szBaseDir;
    g_szBaseDir[259] = '\0';

    void** pCurrentInstance = &pEngineAPI->m_StartupInfo.m_pInstance;
    size_t nInstances = 6;
    do
    {
        pCurrentInstance += 16;
        uint64_t pInstance = *(_QWORD*)&pStartupInfo->m_pInstance;
        pStartupInfo = (StartupInfo_t*)((char*)pStartupInfo + 128);
        *((_QWORD*)pCurrentInstance - 8) = pInstance;
        *((_QWORD*)pCurrentInstance - 7) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[132];
        *((_QWORD*)pCurrentInstance - 6) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[148];
        *((_QWORD*)pCurrentInstance - 5) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[164];
        *((_QWORD*)pCurrentInstance - 4) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[180];
        *((_QWORD*)pCurrentInstance - 3) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[196];
        *((_QWORD*)pCurrentInstance - 2) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[212];
        *((_QWORD*)pCurrentInstance - 1) = *(_QWORD*)&pStartupInfo[-1].m_pParentAppSystemGroup[228];
        --nInstances;
    } while (nInstances);
    *(_QWORD*)pCurrentInstance = *(_QWORD*)&pStartupInfo->m_pInstance;
    *((_QWORD*)pCurrentInstance + 1) = *(_QWORD*)&pStartupInfo->m_szBaseDirectory[8];

    InitVPKSystem();

    v_TRACEINIT(NULL, "COM_InitFilesystem( m_StartupInfo.m_szInitialMod )", "COM_ShutdownFileSystem()");
    COM_InitFilesystem(pEngineAPI->m_StartupInfo.m_szInitialMod);

    *g_bTextMode = true;
}

///////////////////////////////////////////////////////////////////////////////
void SysDll2_Attach()
{
	DetourAttach((LPVOID*)&CEngineAPI_ModInit, &CEngineAPI::VModInit);
	DetourAttach((LPVOID*)&v_CEngineAPI_SetStartupInfo, &CEngineAPI::VSetStartupInfo);
}

void SysDll2_Detach()
{
	DetourDetach((LPVOID*)&CEngineAPI_ModInit, &CEngineAPI::VModInit);
	DetourDetach((LPVOID*)&v_CEngineAPI_SetStartupInfo, &CEngineAPI::VSetStartupInfo);
}