#pragma once

inline CMemory p_CModelLoader__FindModel;
inline auto CModelLoader__FindModel = p_CModelLoader__FindModel.RCast<void* (*)(void* thisptr, const char* pszModelName)>();

inline CMemory p_CModelLoader__LoadModel;
inline auto CModelLoader__LoadModel = p_CModelLoader__LoadModel.RCast<void(*)(void* thisptr, void* mod)>();

inline CMemory p_CModelLoader__UnloadModel;
inline auto CModelLoader__UnloadModel = p_CModelLoader__UnloadModel.RCast<uint64_t(*)(void* thisptr, void* pModel)>();

inline CMemory p_CModelLoader__Studio_LoadModel;
inline auto CModelLoader__Studio_LoadModel = p_CModelLoader__Studio_LoadModel.RCast<void* (*)(void* thisptr)>();

inline CMemory p_CModelLoader__Map_LoadModelGuts;
inline auto CModelLoader__Map_LoadModelGuts = p_CModelLoader__Map_LoadModelGuts.RCast<uint64_t(*)(void* thisptr, void* mod)>();

inline CMemory p_CModelLoader__Map_IsValid;
inline auto CModelLoader__Map_IsValid = p_CModelLoader__Map_IsValid.RCast<bool(*)(void* thisptr, const char* pszMapName)>();

inline CMemory p_GetSpriteInfo;
inline auto GetSpriteInfo = p_GetSpriteInfo.RCast<void* (*)(const char* pName, bool bIsAVI, bool bIsBIK, int& nWidth, int& nHeight, int& nFrameCount, void* a7)>();

inline CMemory p_BuildSpriteLoadName;
inline auto BuildSpriteLoadName = p_BuildSpriteLoadName.RCast<void* (*)(const char* pName, char* pOut, int outLen, bool& bIsAVI, bool& bIsBIK)>();

inline void* g_pModelLoader;

void CModelLoader_Attach();
void CModelLoader_Detach();

///////////////////////////////////////////////////////////////////////////////
class HModelLoader : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CModelLoader::FindModel              : 0x" << std::hex << std::uppercase << p_CModelLoader__FindModel.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::LoadModel              : 0x" << std::hex << std::uppercase << p_CModelLoader__LoadModel.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::UnloadModel            : 0x" << std::hex << std::uppercase << p_CModelLoader__UnloadModel.GetPtr()       << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::Map_LoadModelGuts      : 0x" << std::hex << std::uppercase << p_CModelLoader__Map_LoadModelGuts.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::Map_IsValid            : 0x" << std::hex << std::uppercase << p_CModelLoader__Map_IsValid.GetPtr()       << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::Studio_LoadModel       : 0x" << std::hex << std::uppercase << p_CModelLoader__Studio_LoadModel.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: GetSpriteInfo                        : 0x" << std::hex << std::uppercase << p_GetSpriteInfo.GetPtr()                   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: BuildSpriteLoadName                  : 0x" << std::hex << std::uppercase << p_BuildSpriteLoadName.GetPtr()             << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pModelLoader                       : 0x" << std::hex << std::uppercase << g_pModelLoader                             << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CModelLoader__FindModel         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x41\x55\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxxxxxxxx????");
		p_CModelLoader__LoadModel         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA"), "xxxxxxxx????xxx");
		p_CModelLoader__UnloadModel       = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x58\x18\x55\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA"), "xxxxxxxxxxx????xxx");
		p_CModelLoader__Studio_LoadModel  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxx?xxxxxxxxxxx????");
		p_CModelLoader__Map_LoadModelGuts = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x41\x54\x41\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\xFF\x05\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxx????xxx????xx????"); // BSP.
		p_CModelLoader__Map_IsValid       = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA"), "xxxxxxx????xxx");
		p_GetSpriteInfo                   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x4C\x8B\xAC\x24\x00\x00\x00\x00\xBE\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx????x????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CModelLoader__FindModel         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x41\x57\x48\x83\xEC\x48\x80\x3A\x2A"), "xxxxxxxxxxx");
		p_CModelLoader__LoadModel         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00"), "xxxxxxxx????xxx????");
		p_CModelLoader__UnloadModel       = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED"), "xxxx?xxxx?xxxx????xxxxx");
		p_CModelLoader__Studio_LoadModel  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxxxxxxxx????");
		p_CModelLoader__Map_LoadModelGuts = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x57"), "xxxx?xxxx?xxxxxxxxxx"); // BSP.
		p_CModelLoader__Map_IsValid       = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA\x48\x85\xD2\x0F\x84\x00\x00\x00\x00\x80\x3A\x00\x0F\x84\x00\x00\x00\x00\x4C\x8B\xCA"), "xxxxx????xxxxxxxx????xxxxx????xxx");
		p_GetSpriteInfo                   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x4C\x8B\xBC\x24\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx????");
#endif
		p_BuildSpriteLoadName             = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF1\x48\x8B\xF2"), "xxxx?xxxx?xxxx?xxxx?xxxxx????xxxxxx");

		CModelLoader__FindModel         = p_CModelLoader__FindModel.RCast<void* (*)(void*, const char*)>();
		CModelLoader__LoadModel         = p_CModelLoader__LoadModel.RCast<void(*)(void*, void*)>();
		CModelLoader__UnloadModel       = p_CModelLoader__UnloadModel.RCast<uint64_t(*)(void*, void*)>();
		CModelLoader__Studio_LoadModel  = p_CModelLoader__Studio_LoadModel.RCast<void* (*)(void*)>();
		CModelLoader__Map_LoadModelGuts = p_CModelLoader__Map_LoadModelGuts.RCast<uint64_t(*)(void*, void*)>();
		CModelLoader__Map_IsValid       = p_CModelLoader__Map_IsValid.RCast<bool(*)(void*, const char*)>();
		GetSpriteInfo                   = p_GetSpriteInfo.RCast<void* (*)(const char*, bool, bool, int&, int&, int&, void*)>();
		BuildSpriteLoadName             = p_BuildSpriteLoadName.RCast<void* (*)(const char*, char*, int, bool&, bool&)>();
	}
	virtual void GetVar(void) const
	{
		g_pModelLoader = g_mGameDll.FindPatternSIMD(
			reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x53\x55\x56\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00"),
			"xxxx?xxxxxxxxxxxxxx????").FindPatternSelf("48 ?? 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(3, 7);
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HModelLoader);
