//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/jobthread.h"
#include "engine/host_cmd.h"
#include "engine/sys_utils.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_utils.h"
#include "rtech/rtech_game.h"
#include "datacache/mdlcache.h"
#include "filesystem/filesystem.h"

string g_svLevelName;
bool s_bLevelResourceInitialized = false;
bool s_bBasePaksInitialized = false;
//-----------------------------------------------------------------------------
// Purpose: checks if level has changed
// Input  : &svLevelName - 
// Output : true if level name deviates from previous level
//-----------------------------------------------------------------------------
bool MOD_LevelHasChanged(const string& svLevelName)
{
	return (strcmp(svLevelName.c_str(), g_svLevelName.c_str()) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: gets the queued pak handles
// Input  : *a1 - 
//          *a2 - 
//          a3 - 
// Output : __int64
//-----------------------------------------------------------------------------
__int64 __fastcall MOD_GetQueuedPakHandle(char* a1, char* a2, __int64 a3)
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
void MOD_ProcessPakQueue()
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
    __int64 v23; // rbx
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
    if ((*(unsigned __int8(__fastcall**)(__int64))(*(_QWORD*)*(_QWORD*)g_pFileSystem_Stdio + 696i64 - FSTDIO_OFS))(*(_QWORD*)g_pFileSystem_Stdio) && !*dword_1634F445C)
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
                        g_pakLoadApi->Unload(*(_DWORD*)v10);
                        MOD_UnloadPakFile();
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
            MOD_GetQueuedPakHandle(v17, *((char**)v15 + 34), 260i64);
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
                    v23 = *qword_1671061C8;
                    if (*qword_1671061C8)
                    {
                        if (!*(_BYTE*)(*qword_1671061C8 + 4))
                        {
                            if (*qword_167ED7BC0 || WORD2(*qword_167ED7C68) != HIWORD(*qword_167ED7C68))
                            {
                                if (!JT_AcquireFifoLock((JobFifoLock_s*)&*qword_167ED7BE0)
                                    && !(unsigned __int8)sub_14045BAC0((__int64(__fastcall*)(__int64, _DWORD*, __int64, _QWORD*))qword_14045C070, (__int64)&*qword_167ED7BE0, -1i64, 0i64))
                                {
                                    sub_14045A1D0((unsigned __int8(__fastcall*)(_QWORD))qword_14045C070, (__int64)&*qword_167ED7BE0, -1i64, 0i64, 0i64, 1);
                                }

                                sub_140441220(v25, v24);
                                if (GetCurrentThreadId() == *dword_1641E443C)
                                {
                                    if (*byte_167208B0C)
                                    {
                                        *byte_167208B0C = 0;
                                        JT_ReleaseFifoLock((JobFifoLock_s*)&*qword_167ED7BE0);
                                    }
                                }
                                JT_ReleaseFifoLock((JobFifoLock_s*)&*qword_167ED7BE0);
                                v23 = *qword_1671061C8;
                            }
                            (*(void(__fastcall**)(__int64, __int64))(*(_QWORD*)*(_QWORD*)g_pFileSystem_Stdio + 656i64 - FSTDIO_OFS))(*(_QWORD*)g_pFileSystem_Stdio, 256i64);
                            (*(void(__fastcall**)(__int64, __int64))(*(_QWORD*)*(_QWORD*)g_pFileSystem_Stdio + 648i64 - FSTDIO_OFS))(*(_QWORD*)g_pFileSystem_Stdio, v23);
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
            s_bLevelResourceInitialized = true;
            MOD_PreloadPakFile(g_svLevelName);
        }
        *(_DWORD*)v15 = g_pakLoadApi->AsyncLoad(v17, g_pMallocPool.GetPtr(), 4, 0);

        if (strcmp(v17, "common_mp.rpak") == 0 || strcmp(v17, "common_sp.rpak") == 0 || strcmp(v17, "common_pve.rpak") == 0)
        {
            RPakHandle_t pakHandle = g_pakLoadApi->AsyncLoad("common_sdk.rpak", g_pMallocPool.GetPtr(), 4, 0);
            if (pakHandle != -1)
                g_LoadedPakHandle.push_back(pakHandle);
        }
        if (strcmp(v17, "ui_mp.rpak") == 0)
        {
            RPakHandle_t pakHandle = g_pakLoadApi->AsyncLoad("ui_sdk.rpak", g_pMallocPool.GetPtr(), 4, 0);
            if (pakHandle != -1)
                g_LoadedPakHandle.push_back(pakHandle);
        }

    LABEL_37:
        v21 = *(_DWORD*)v15;
        if (*(_DWORD*)v15 != -1)
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
bool MOD_LoadPakForMap(const char* szLevelName)
{
	if (MOD_LevelHasChanged(szLevelName))
		s_bLevelResourceInitialized = false;

	g_svLevelName = szLevelName;
	return v_MOD_LoadPakForMap(szLevelName);
}

//-----------------------------------------------------------------------------
// Purpose: loads required pakfile assets for specified BSP
// Input  : &svSetFile - 
//-----------------------------------------------------------------------------
void MOD_PreloadPakFile(const string& svLevelName)
{
	ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << svLevelName << ".json";

	fs::path fsPath = fs::current_path() /= ostream.str();
	if (FileExists(fsPath.string().c_str()))
	{
		nlohmann::json jsIn;
		try
		{
			ifstream iPakLoadDefFile(fsPath.string().c_str(), std::ios::binary); // Load prerequisites file.

			jsIn = nlohmann::json::parse(iPakLoadDefFile);
			iPakLoadDefFile.close();

			if (!jsIn.is_null())
			{
				if (!jsIn["rpak"].is_null())
				{
					for (auto& it : jsIn["rpak"])
					{
						if (it.is_string())
						{
							string svToLoad = it.get<string>() + ".rpak";
							RPakHandle_t nPakId = g_pakLoadApi->AsyncLoad(svToLoad.c_str(), g_pMallocPool.GetPtr(), 4, 0);

							if (nPakId == -1)
								Error(eDLL_T::ENGINE, "%s: unable to load pak '%s' results '%d'\n", __FUNCTION__, svToLoad.c_str(), nPakId);
							else
								g_LoadedPakHandle.push_back(nPakId);
						}
					}
				}
			}
		}
		catch (const std::exception& ex)
		{
			Warning(eDLL_T::RTECH, "Exception while parsing RPak load list: '%s'\n", ex.what());
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: unloads all pakfiles loaded by the SDK
//-----------------------------------------------------------------------------
void MOD_UnloadPakFile(void)
{
	for (auto& it : g_LoadedPakHandle)
	{
		if (it >= 0)
		{
			g_pakLoadApi->Unload(it);
		}
	}
	g_LoadedPakHandle.clear();
	g_BadMDLHandles.clear();
}

void CModelBsp_Attach()
{
	DetourAttach((LPVOID*)&v_MOD_LoadPakForMap, &MOD_LoadPakForMap);
	DetourAttach((LPVOID*)&v_MOD_ProcessPakQueue, &MOD_ProcessPakQueue);
}

void CModelBsp_Detach()
{
	DetourDetach((LPVOID*)&v_MOD_LoadPakForMap, &MOD_LoadPakForMap);
	DetourDetach((LPVOID*)&v_MOD_ProcessPakQueue, &MOD_ProcessPakQueue);
}