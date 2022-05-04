//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/jobthread.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/sys_utils.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_game.h"

//-----------------------------------------------------------------------------
// Purpose: loads required pakfile assets for specified BSP
// Input  : svSetFile - 
//-----------------------------------------------------------------------------
void MOD_PreloadPak()
{
	ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << g_pHostState->m_levelName << ".json";

	fs::path fsPath = std::filesystem::current_path() /= ostream.str();
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
// Purpose: load assets for level with fifolock (still not reliable enough).
// Input  : svSetFile - 
// TODO   : Rebuild '0x140341D40' and load paks from there, this should always work.
//-----------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
bool MOD_LoadPakForMap()
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
bool MOD_LoadPakForMap(void* pBuffer)
#endif
{
	if (!g_bLevelResourceInitialized &&
		g_bBasePaksInitialized)
	{
		g_bLevelResourceInitialized = true;

		if (g_pHostState->LevelHasChanged())
		{
			JT_AcquireFifoLock();
			MOD_PreloadPak();
		}
	}

	return v_MOD_LoadPakForMap(pBuffer);

	//void* v1; // r8
	//const char* append_rpak_var; // rdx
	//char* result; // rax
	//int v4; // ecx
	//int v5; // edx
	//__int64 v6; // rax
	//char v7; // cl
	//int v8; // ebx
	//char rpak_name_var[272]; // [rsp+20h] [rbp-118h] BYREF

	//static auto unk_14D475233 = CMemory(0x141744E70).RCast<void*>();
	//static auto byte_14D475220 = CMemory(0x14D475220).RCast<char(*)[19]>();
	//static auto byte_1666ECF20 = CMemory(0x1666ECF20).RCast<char*>();
	//static auto dword_141717BB8 = CMemory(0x141717BB8).RCast<int*>();

	//static auto sub_14023BDD0 = CMemory(0x14023BDD0).RCast<__int64(*)()>();
	//static auto sub_1404418A0 = CMemory(0x1404418A0).RCast<__int64(*)(int)>();
	//static auto sub_140441520 = CMemory(0x140441520).RCast<__int64(*)(int, void*)>();

	//v1 = &*(void**)unk_14D475233;
	//append_rpak_var = "%s.rpak";
	//if (!*byte_14D475220[0])
	//	v1 = pBuffer;
	//if (!*byte_14D475220[0])
	//	append_rpak_var = "%s_loadscreen.rpak";
	//sprintf(rpak_name_var, append_rpak_var, v1);
	//result = byte_1666ECF20;
	//do
	//{
	//	v4 = (unsigned __int8)result[rpak_name_var - byte_1666ECF20];
	//	v5 = (unsigned __int8)*result - v4;
	//	if (v5)
	//		break;
	//	++result;
	//} while (v4);
	//if (v5)
	//{
	//	v6 = 0i64;                                  // copying rpak name into byte buffer
	//	do
	//	{
	//		v7 = rpak_name_var[v6];
	//		byte_1666ECF20[v6++] = v7;
	//	} while (v7);
	//	sub_14023BDD0();
	//	if (*dword_141717BB8 != -1)
	//		sub_1404418A0(*dword_141717BB8);

	//	if (!g_bLevelResourceInitialized &&
	//		g_bBasePaksInitialized)
	//		MOD_PreloadPak();
	//	result = (char*)g_pakLoadApi->AsyncLoad(rpak_name_var);

	//	v8 = (int)result;
	//	if ((_DWORD)result != -1)
	//	{
	//		result = (char*)sub_140441520((unsigned int)result, nullptr);
	//		if (!(_BYTE)result)
	//			v8 = -1;
	//	}
	//	*dword_141717BB8 = v8;
	//}
	//return result;
}


void CModelBsp_Attach()
{
	DetourAttach((LPVOID*)&v_MOD_LoadPakForMap, &MOD_LoadPakForMap);
}

void CModelBsp_Detach()
{
	DetourDetach((LPVOID*)&v_MOD_LoadPakForMap, &MOD_LoadPakForMap);
}