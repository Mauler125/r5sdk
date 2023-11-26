//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
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
#include "engine/sys_engine.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/traceinit.h"
#ifndef DEDICATED
#include "engine/sys_mainwind.h"
#include "materialsystem/cmaterialsystem.h"
#include "windows/id3dx.h"
#include "client/vengineclient_impl.h"
#include "geforce/reflex.h"
#endif // !DEDICATED
#include "rtech/rtech_utils.h"
#include "filesystem/filesystem.h"
constexpr char DFS_ENABLE_PATH[] = "/vpk/enable.txt";

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
    char szCacheEnableFilePath[260]; // [rsp+20h] [rbp-118h] BYREF
    char bFixSlashes = FileSystem()->GetCurrentDirectory(szCacheEnableFilePath, sizeof(szCacheEnableFilePath)) ? szCacheEnableFilePath[0] : '\0';

    size_t nCachePathLen = strlen(szCacheEnableFilePath);
    size_t nCacheFileLen = sizeof(DFS_ENABLE_PATH)-1;

    if ((nCachePathLen + nCacheFileLen) < 0x104 || (nCacheFileLen = (sizeof(szCacheEnableFilePath)-1) - nCachePathLen, nCachePathLen != (sizeof(szCacheEnableFilePath)-1)))
    {
        strncat(szCacheEnableFilePath, DFS_ENABLE_PATH, nCacheFileLen)[sizeof(szCacheEnableFilePath)-1] = '\0';
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
#else // Dedicated runs server vpk's and must have 'vpk/mp_common.bsp' mounted.
        FileSystem()->SetVPKCacheModeServer();
        FileSystem()->MountVPKFile("vpk/server_mp_common.bsp");
#endif // !DEDICATED
    }
}

InitReturnVal_t CEngineAPI::VInit(CEngineAPI* pEngineAPI)
{
    return CEngineAPI_Init(pEngineAPI);
}

//-----------------------------------------------------------------------------
// Initialization, shutdown of a mod.
//-----------------------------------------------------------------------------
bool CEngineAPI::VModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)
{
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
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
    if (*g_bTextMode)
    {
        return;
    }

    const size_t nBufLen = sizeof(pStartupInfo->m_szBaseDirectory);
    strncpy(g_szBaseDir, pStartupInfo->m_szBaseDirectory, nBufLen);

    g_pEngineParms->baseDirectory = g_szBaseDir;
    g_szBaseDir[nBufLen-1] = '\0';

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
    v_COM_InitFilesystem(pEngineAPI->m_StartupInfo.m_szInitialMod);

    *g_bTextMode = true;
#else
    // !TODO: 'TRACEINIT' needs to be reimplemented in S0/S1 (inline).
    v_CEngineAPI_SetStartupInfo(pEngineAPI, pStartupInfo);
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1)
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEngineAPI::PumpMessages()
{
#ifndef DEDICATED
    CEngineAPI_PumpMessages();
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEngineAPI::MainLoop()
{
    // Main message pump
    while (true)
    {
        // Pump messages unless someone wants to quit
        if (g_pEngine->GetQuitting() != IEngine::QUIT_NOTQUITTING)
        {
            if (g_pEngine->GetQuitting() != IEngine::QUIT_TODESKTOP)
                return true;

            return false;
        }

#ifndef DEDICATED
        const MaterialAdapterInfo_t& adapterInfo = g_pMaterialAdapterMgr->GetAdapterInfo();

        // Only run on NVIDIA display drivers; AMD and Intel are not
        // supported by NVIDIA Reflex.
        if (adapterInfo.m_VendorID == NVIDIA_VENDOR_ID)
        {
            if (GFX_HasPendingLowLatencyParameterUpdates())
            {
                const bool bUseLowLatencyMode = gfx_nvnUseLowLatency->GetBool();
                const bool bUseLowLatencyBoost = gfx_nvnUseLowLatencyBoost->GetBool();

                float fpsMax = fps_max_gfx->GetFloat();

                if (fpsMax == -1.0f)
                {
                    const float globalFps = fps_max->GetFloat();

                    // Make sure the global fps limiter is 'unlimited'
                    // before we let the gfx frame limiter cap it to
                    // the desktop's refresh rate; not adhering to
                    // this will result in a major performance drop.
                    if (globalFps == 0.0f)
                        fpsMax = g_pGame->GetTVRefreshRate();
                    else
                        fpsMax = 0.0f; // Don't let NVIDIA limit the frame rate.
                }

                GFX_UpdateLowLatencyParameters(D3D11Device(), bUseLowLatencyMode,
                    bUseLowLatencyBoost, false, fpsMax);
            }

            GFX_RunLowLatencyFrame(D3D11Device());
        }

        CEngineAPI::PumpMessages();
#endif // !DEDICATED

        if (g_pEngine->Frame())
        {
#ifndef DEDICATED
            // Only increment frame number if we ran an actual engine frame.
            GFX_IncrementFrameNumber();
#endif // !DEDICATED
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void VSys_Dll2::Detour(const bool bAttach) const
{
	DetourSetup(&CEngineAPI_Init, &CEngineAPI::VInit, bAttach);
	DetourSetup(&CEngineAPI_ModInit, &CEngineAPI::VModInit, bAttach);
	DetourSetup(&CEngineAPI_PumpMessages, &CEngineAPI::PumpMessages, bAttach);
	DetourSetup(&CEngineAPI_MainLoop, &CEngineAPI::MainLoop, bAttach);
	DetourSetup(&v_CEngineAPI_SetStartupInfo, &CEngineAPI::VSetStartupInfo, bAttach);
}
