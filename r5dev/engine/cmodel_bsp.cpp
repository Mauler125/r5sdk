//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/memstd.h"
#include "tier0/jobthread.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_utils.h"
#include "rtech/rtech_game.h"
#include "vpc/keyvalues.h"
#include "datacache/mdlcache.h"
#include "filesystem/filesystem.h"

vector<string> g_vAllMaps;
string s_svLevelName;
bool s_bLevelResourceInitialized = false;
bool s_bBasePaksInitialized = false;
KeyValues* s_pLevelSetKV = nullptr;

//-----------------------------------------------------------------------------
// Purpose: checks if level has changed
// Input  : *pszLevelName - 
// Output : true if level name deviates from previous level
//-----------------------------------------------------------------------------
bool Mod_LevelHasChanged(const char* pszLevelName)
{
	return (s_svLevelName.compare(pszLevelName) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: gets all installed maps
//-----------------------------------------------------------------------------
void Mod_GetAllInstalledMaps()
{
    std::lock_guard<std::mutex> l(g_MapVecMutex);
    g_vAllMaps.clear(); // Clear current list.

    fs::directory_iterator fsDir("vpk");
    std::regex rgArchiveRegex{ R"([^_]*_(.*)(.bsp.pak000_dir).*)" };
    std::smatch smRegexMatches;

    for (const fs::directory_entry& dEntry : fsDir)
    {
        std::string svFileName = dEntry.path().u8string();
        std::regex_search(svFileName, smRegexMatches, rgArchiveRegex);

        if (!smRegexMatches.empty())
        {
            if (smRegexMatches[1].str().compare("frontend") == 0)
                continue; // Frontend contains no BSP's.

            else if (smRegexMatches[1].str().compare("mp_common") == 0)
            {
                if (std::find(g_vAllMaps.begin(), g_vAllMaps.end(), "mp_lobby") == g_vAllMaps.end())
                    g_vAllMaps.push_back("mp_lobby");
                continue; // Common contains mp_lobby.
            }

            if (std::find(g_vAllMaps.begin(), g_vAllMaps.end(), smRegexMatches[1].str()) == g_vAllMaps.end())
                g_vAllMaps.push_back(smRegexMatches[1].str());
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: gets the queued pak handles
// Input  : *a1 - 
//          *a2 - 
//          a3 - 
// Output : __int64
//-----------------------------------------------------------------------------
__int64 __fastcall Mod_GetQueuedPakHandle(char* a1, char* a2, __int64 a3)
{
    char v3; // al
    signed int v4; // er11
    __int64 v5; // r10
    char* v6; // r9
    signed __int64 v7; // rdx
    char v8; // al
    char* v10; // r8
    char* v11; // r8

    v3 = *a2;
    v4 = 0;
    *a1 = *a2;
    v5 = 0i64;
    if (v3)
    {
        v6 = a1;
        v7 = a2 - a1;
        while (1)
        {
            ++v5;
            ++v6;
            if (v5 == a3)
                break;
            v8 = v6[v7];
            *v6 = v8;
            if (!v8)
                return v5;
        }
        *(v6 - 1) = 0;
        if (--v5)
        {
            v10 = &a1[v5 - 1];
            if ((*v10 & 0xC0) == 0x80)
            {
                do
                    ++v4;
                while ((v10[-v4] & 0xC0) == 0x80);
            }
            v11 = &v10[-v4];
            if (v4 != ((0xE5000000 >> (((unsigned __int8)*v11 >> 3) & 0x1E)) & 3))
            {
                *v11 = 0;
                v5 -= v4;
            }
        }
    }
    return v5;
}

//-----------------------------------------------------------------------------
// Purpose: processes queued pak files
//-----------------------------------------------------------------------------
void Mod_ProcessPakQueue()
{
    char v0; // bl
    char** v1; // r10
    int i; // er9
    char* v3; // rcx
    signed __int64 v4; // r8
    int v5; // eax
    int v6; // edx
    int v7; // eax
    __int64 v8; // rbp
    __int64 v9; // rsi
    char* v10; // rbx
    unsigned int v11; // ecx
    __int64 v12; // rax
    int v13; // edi
    char v14; // al
    char* v15; // rbx
    int v16; // edi
    char* v17; // rsi
    char* v18; // rax
    int v19; // ecx
    int v20; // er8
    int v21; // ecx
    __int64 v22; // rdx
    __int64 v24{}; // rdx
    __int64 v25{}; // rcx

    v0 = 0;
    if (*(float*)&*dword_14B383420 == 1.0 && *qword_167ED7BB8 && *((int*)&*qword_14180A098/* + 36*/) < 2)
    {
        *byte_16709DDDF = 0;
        v0 = 1;
    }
    else if (*byte_16709DDDF)
    {
        return;
    }
    if (FileSystem()->ResetItemCache() && !*dword_1634F445C)
    {
        v1 = &*off_141874660;
        for (i = 0; i < 5; ++i)
        {
            if (*((_BYTE*)v1 - 268))
                break;
            v3 = (char*)&*unk_141874555 + 280 * i;
            v4 = *v1 - v3;
            do
            {
                v5 = (unsigned __int8)v3[v4];
                v6 = (unsigned __int8)*v3 - v5;
                if (v6)
                    break;
                ++v3;
            }       while (v5);
            if (v6)
                break;
            v1 += 35;
        }
        v7 = 0;
        if (!v0)
            v7 = i;
        v8 = v7;
        if (v7 <= 4i64)
        {
            v9 = 4i64;
            v10 = (char*)&*unk_1418749B0;
            do
            {
                if (v10[5])
                {
                    v11 = *(_DWORD*)v10;
                    v12 = *(_DWORD*)v10 & 0x1FF;
                    v10[4] = 1;
                    if (*((_DWORD*)&*g_pLoadedPakInfo + 46 * v12) == v11)
                    {
                        v13 = *((_DWORD*)&*g_pLoadedPakInfo + 46 * v12 + 1);
                        v14 = v10[4];
                    }
                    else
                    {
                        v13 = 14;
                        v14 = 1;
                    }
                    if (!v14 || v13 == 9)
                    {
                        // SDK pak files must be unloaded before the engine pak files,
                        // as we reference assets within engine pak files.
                        const RPakLoadedInfo_t* pLoadedPakInfo = g_pRTech->GetPakLoadedInfo(*(RPakHandle_t*)v10);
                        if (pLoadedPakInfo)
                        {
                            const char* pszLoadedPakName = pLoadedPakInfo->m_pszFileName;

                            if (strcmp(pszLoadedPakName, "common_mp.rpak") == 0 ||
                                strcmp(pszLoadedPakName, "common_sp.rpak") == 0 ||
                                strcmp(pszLoadedPakName, "common_pve.rpak") == 0)
                            {
                                const RPakLoadedInfo_t* pLoadedSdkPak = g_pRTech->GetPakLoadedInfo("common_sdk.rpak");

                                if (pLoadedSdkPak) // Only unload if sdk pak file is loaded.
                                    g_pakLoadApi->UnloadPak(pLoadedSdkPak->m_nHandle);

                            }
                            else if (strcmp(pszLoadedPakName, "ui_mp.rpak") == 0)
                            {
                                const RPakLoadedInfo_t* pLoadedSdkPak = g_pRTech->GetPakLoadedInfo("ui_sdk.rpak");

                                if (pLoadedSdkPak) // Only unload if sdk pak file is loaded.
                                    g_pakLoadApi->UnloadPak(pLoadedSdkPak->m_nHandle);
                            }
                        }

                        g_pakLoadApi->UnloadPak(*(RPakHandle_t*)v10);
                        Mod_UnloadPakFile(); // Unload mod pak files.

                        if (s_pLevelSetKV)
                        {
                            // Delete current level settings if we drop all paks..
                            s_pLevelSetKV->DeleteThis();
                            s_pLevelSetKV = nullptr;
                        }
                    }
                    if (v13 && (unsigned int)(v13 - 13) > 1)
                        return;
                    *((_WORD*)v10 + 2) = 0;
                    *(_DWORD*)v10 = -1;
                }
                --v9;
                v10 -= 280;
            }       while (v9 >= v8);
        }
        *byte_16709DDDF = 1;
        v15 = (char*)&*unk_141874550;
        v16 = 0;
        while (1)
        {
            v17 = (char*)&*unk_141874550 + 280 * v16 + 5;
            v18 = v17;
            do
            {
                v19 = (unsigned __int8)v18[*((_QWORD*)v15 + 34) - (_QWORD)v17];
                v20 = (unsigned __int8)*v18 - v19;
                if (v20)
                    break;
                ++v18;
            }       while (v19);
            if (!v20)
                goto LABEL_37;
            Mod_GetQueuedPakHandle(v17, *((char**)v15 + 34), 260i64);
            if (v15[5])
                break;
            *(_DWORD*)v15 = -1;
        LABEL_40:
            ++v16;
            v15 += 280;
            if (v16 >= 5)
            {
                if (*byte_16709DDDF)
                {
                    if (*g_pMTVFTaskItem)
                    {
                        if (!*(_BYTE*)(*g_pMTVFTaskItem + 4))
                        {
                            if (*qword_167ED7BC0 || WORD2(*qword_167ED7C68) != HIWORD(*qword_167ED7C68))
                            {
                                if (!JT_AcquireFifoLock(&*g_pPakFifoLock)
                                    && !(unsigned __int8)sub_14045BAC0((__int64(__fastcall*)(__int64, _DWORD*, __int64, _QWORD*))g_pPakFifoLockWrapper, &*g_pPakFifoLock, -1i64, 0i64))
                                {
                                    sub_14045A1D0((unsigned __int8(__fastcall*)(_QWORD))g_pPakFifoLockWrapper, &*g_pPakFifoLock, -1i64, 0i64, 0i64, 1);
                                }

                                sub_140441220(v25, v24);
                                if (ThreadInMainThread())
                                {
                                    if (*g_bPakFifoLockAcquired)
                                    {
                                        *g_bPakFifoLockAcquired = 0;
                                        JT_ReleaseFifoLock(&*g_pPakFifoLock);
                                    }
                                }
                                JT_ReleaseFifoLock(&*g_pPakFifoLock);
                            }
                            FileSystem()->ResetItemCacheSize(256);
                            FileSystem()->PrecacheTaskItem(*g_pMTVFTaskItem);
                        }
                    }
                }
                return;
            }
        }
        if (strcmp(v17, "mp_lobby.rpak") == 0)
            s_bBasePaksInitialized = true;

        if (s_bBasePaksInitialized && !s_bLevelResourceInitialized)
        {
            Mod_PreloadLevelPaks(s_svLevelName.c_str());
            s_bLevelResourceInitialized = true;
        }
        *(_DWORD*)v15 = g_pakLoadApi->LoadAsync(v17, g_pMallocPool, 4, 0);

        if (strcmp(v17, "common_mp.rpak") == 0 || strcmp(v17, "common_sp.rpak") == 0 || strcmp(v17, "common_pve.rpak") == 0)
            RPakHandle_t pakHandle = g_pakLoadApi->LoadAsync("common_sdk.rpak", g_pMallocPool, 4, 0);
        if (strcmp(v17, "ui_mp.rpak") == 0)
            RPakHandle_t pakHandle = g_pakLoadApi->LoadAsync("ui_sdk.rpak", g_pMallocPool, 4, 0);

    LABEL_37:
        v21 = *(_DWORD*)v15;
        if (*(_DWORD*)v15 != INVALID_PAK_HANDLE)
        {
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
            v22 = 232i64 * (v21 & 0x1FF);
#else
            v22 = 184i64 * (v21 & 0x1FF);
#endif
            if (*(_DWORD*)((char*)&*g_pLoadedPakInfo + v22) != v21 || ((*(_DWORD*)((char*)&*g_pLoadedPakInfo + v22 + 4) - 9) & 0xFFFFFFFB) != 0)
            {
                *byte_16709DDDF = 0;                return;
            }
        }
        goto LABEL_40;
    }
}

//-----------------------------------------------------------------------------
// Purpose: load assets for level with fifolock.
// Input  : *szLevelName - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool Mod_LoadPakForMap(const char* pszLevelName)
{
	if (Mod_LevelHasChanged(pszLevelName))
		s_bLevelResourceInitialized = false;

	s_svLevelName = pszLevelName;
	return v_Mod_LoadPakForMap(pszLevelName);
}

//-----------------------------------------------------------------------------
// Purpose: loads the level settings file, returns current if level hasn't changed.
// Input  : *pszLevelName - 
// Output : KeyValues*
//-----------------------------------------------------------------------------
KeyValues* Mod_GetLevelSettings(const char* pszLevelName)
{
    if (s_pLevelSetKV)
    {
        if (s_bLevelResourceInitialized)
            return s_pLevelSetKV;

        s_pLevelSetKV->DeleteThis();
    }

    char szPathBuffer[MAX_PATH];
    snprintf(szPathBuffer, sizeof(szPathBuffer), "scripts/levels/settings/%s.kv", pszLevelName);

    s_pLevelSetKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_LEVELSETTINGS, szPathBuffer, "GAME");
    return s_pLevelSetKV;
}

//-----------------------------------------------------------------------------
// Purpose: loads required pakfile assets for specified BSP level
// Input  : &svSetFile - 
//-----------------------------------------------------------------------------
void Mod_PreloadLevelPaks(const char* pszLevelName)
{
    KeyValues* pSettingsKV = Mod_GetLevelSettings(pszLevelName);

    if (!pSettingsKV)
        return;

    KeyValues* pPakListKV = pSettingsKV->FindKey("PakList");

    if (!pPakListKV)
        return;

    char szPathBuffer[MAX_PATH];

    for (KeyValues* pSubKey = pPakListKV->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
    {
        if (!pSubKey->GetBool())
            continue;

        snprintf(szPathBuffer, sizeof(szPathBuffer), "%s.rpak", pSubKey->GetName());
        RPakHandle_t nPakId = g_pakLoadApi->LoadAsync(szPathBuffer, g_pMallocPool, 4, 0);

        if (nPakId == INVALID_PAK_HANDLE)
            Error(eDLL_T::ENGINE, NO_ERROR, "%s: unable to load pak '%s' results '%d'\n", __FUNCTION__, szPathBuffer, nPakId);
        else
            g_vLoadedPakHandle.push_back(nPakId);
    }
}

//-----------------------------------------------------------------------------
// Purpose: unloads all pakfiles loaded by the SDK
//-----------------------------------------------------------------------------
void Mod_UnloadPakFile(void)
{
	for (const RPakHandle_t& it : g_vLoadedPakHandle)
	{
		if (it >= 0)
		{
			g_pakLoadApi->UnloadPak(it);
		}
	}
	g_vLoadedPakHandle.clear();
	g_vBadMDLHandles.clear();
}

void CModelBsp_Attach()
{
	DetourAttach((LPVOID*)&v_Mod_LoadPakForMap, &Mod_LoadPakForMap);
	DetourAttach((LPVOID*)&v_Mod_ProcessPakQueue, &Mod_ProcessPakQueue);
}

void CModelBsp_Detach()
{
	DetourDetach((LPVOID*)&v_Mod_LoadPakForMap, &Mod_LoadPakForMap);
	DetourDetach((LPVOID*)&v_Mod_ProcessPakQueue, &Mod_ProcessPakQueue);
}