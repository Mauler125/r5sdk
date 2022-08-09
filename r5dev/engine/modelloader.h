#pragma once
#include "engine/gl_model_private.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class IModelLoader
{
public:
	enum REFERENCETYPE
	{
		// The name is allocated, but nothing else is in memory or being referenced
		FMODELLOADER_NOTLOADEDORREFERENCED = 0,
		// The model has been loaded into memory
		FMODELLOADER_LOADED = (1 << 0),

		// The model is being referenced by the server code
		FMODELLOADER_SERVER = (1 << 1),
		// The model is being referenced by the client code
		FMODELLOADER_CLIENT = (1 << 2),
		// The model is being referenced in the client .dll
		FMODELLOADER_CLIENTDLL = (1 << 3),
		// The model is being referenced by static props
		FMODELLOADER_STATICPROP = (1 << 4),
		// The model is a detail prop
		FMODELLOADER_DETAILPROP = (1 << 5),
		// The model is the simple version of the world geometry
		FMODELLOADER_SIMPLEWORLD = (1 << 6),
		// The model is dynamically loaded
		FMODELLOADER_DYNSERVER = (1 << 7),
		FMODELLOADER_DYNCLIENT = (1 << 8),
		FMODELLOADER_COMBINED = (1 << 9),
		FMODELLOADER_DYNAMIC = FMODELLOADER_DYNSERVER | FMODELLOADER_DYNCLIENT | FMODELLOADER_COMBINED,

		FMODELLOADER_REFERENCEMASK = (FMODELLOADER_SERVER | FMODELLOADER_CLIENT | FMODELLOADER_CLIENTDLL | FMODELLOADER_STATICPROP | FMODELLOADER_DETAILPROP | FMODELLOADER_DYNAMIC | FMODELLOADER_SIMPLEWORLD),

		// The model was touched by the preload method
		FMODELLOADER_TOUCHED_BY_PRELOAD = (1 << 15),
		// The model was loaded by the preload method, a postload fixup is required
		FMODELLOADER_LOADED_BY_PRELOAD = (1 << 16),
		// The model touched its materials as part of its load
		FMODELLOADER_TOUCHED_MATERIALS = (1 << 17),
	};
};

class CModelLoader
{
public:
	static void LoadModel(CModelLoader* loader, model_t* model);
	static uint64_t Map_LoadModelGuts(CModelLoader* loader, model_t* model);
};

inline CMemory p_CModelLoader__FindModel;
inline auto CModelLoader__FindModel = p_CModelLoader__FindModel.RCast<void* (*)(CModelLoader* loader, const char* pszModelName)>();

inline CMemory p_CModelLoader__LoadModel;
inline auto CModelLoader__LoadModel = p_CModelLoader__LoadModel.RCast<void(*)(CModelLoader* loader, model_t* model)>();

inline CMemory p_CModelLoader__UnloadModel;
inline auto CModelLoader__UnloadModel = p_CModelLoader__UnloadModel.RCast<uint64_t(*)(CModelLoader* loader, model_t* model)>();

inline CMemory p_CModelLoader__Studio_LoadModel;
inline auto CModelLoader__Studio_LoadModel = p_CModelLoader__Studio_LoadModel.RCast<void* (*)(CModelLoader* loader)>();

inline CMemory p_CModelLoader__Map_LoadModelGuts;
inline auto CModelLoader__Map_LoadModelGuts = p_CModelLoader__Map_LoadModelGuts.RCast<uint64_t(*)(CModelLoader* loader, model_t* model)>();

inline CMemory p_CModelLoader__Map_IsValid;
inline auto CModelLoader__Map_IsValid = p_CModelLoader__Map_IsValid.RCast<bool(*)(CModelLoader* loader, const char* pszMapName)>();

inline CMemory p_GetSpriteInfo;
inline auto GetSpriteInfo = p_GetSpriteInfo.RCast<void* (*)(const char* pName, bool bIsAVI, bool bIsBIK, int& nWidth, int& nHeight, int& nFrameCount, void* a7)>();

inline CMemory p_BuildSpriteLoadName;
inline auto BuildSpriteLoadName = p_BuildSpriteLoadName.RCast<void* (*)(const char* pName, char* pOut, int outLen, bool& bIsAVI, bool& bIsBIK)>();

inline CModelLoader* g_pModelLoader;

void CModelLoader_Attach();
void CModelLoader_Detach();

///////////////////////////////////////////////////////////////////////////////
class VModelLoader : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CModelLoader::FindModel              : {:#18x} |\n", p_CModelLoader__FindModel.GetPtr());
		spdlog::debug("| FUN: CModelLoader::LoadModel              : {:#18x} |\n", p_CModelLoader__LoadModel.GetPtr());
		spdlog::debug("| FUN: CModelLoader::UnloadModel            : {:#18x} |\n", p_CModelLoader__UnloadModel.GetPtr());
		spdlog::debug("| FUN: CModelLoader::Map_LoadModelGuts      : {:#18x} |\n", p_CModelLoader__Map_LoadModelGuts.GetPtr());
		spdlog::debug("| FUN: CModelLoader::Map_IsValid            : {:#18x} |\n", p_CModelLoader__Map_IsValid.GetPtr());
		spdlog::debug("| FUN: CModelLoader::Studio_LoadModel       : {:#18x} |\n", p_CModelLoader__Studio_LoadModel.GetPtr());
		spdlog::debug("| FUN: GetSpriteInfo                        : {:#18x} |\n", p_GetSpriteInfo.GetPtr());
		spdlog::debug("| FUN: BuildSpriteLoadName                  : {:#18x} |\n", p_BuildSpriteLoadName.GetPtr());
		spdlog::debug("| VAR: g_pModelLoader                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pModelLoader));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CModelLoader__FindModel         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x41\x55\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxxxxxxxx????");
		p_CModelLoader__LoadModel         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xFA"), "xxxxxxxx????xxx");
		p_CModelLoader__UnloadModel       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x58\x18\x55\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA"), "xxxxxxxxxxx????xxx");
		p_CModelLoader__Studio_LoadModel  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00"), "xxxx?xxxxxxxxxxx????");
		p_CModelLoader__Map_LoadModelGuts = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x41\x54\x41\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\xFF\x05\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxx????xxx????xx????"); // BSP.
		p_CModelLoader__Map_IsValid       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA"), "xxxxxxx????xxx");
		p_GetSpriteInfo                   = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x4C\x8B\xAC\x24\x00\x00\x00\x00\xBE\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx????x????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CModelLoader__FindModel         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x41\x57\x48\x83\xEC\x48\x80\x3A\x2A"), "xxxxxxxxxxx");
		p_CModelLoader__LoadModel         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00"), "xxxxxxxx????xxx????");
		p_CModelLoader__UnloadModel       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED"), "xxxx?xxxx?xxxx????xxxxx");
		p_CModelLoader__Studio_LoadModel  = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00"), "xxxx?xxxxxxxxxx????");
		p_CModelLoader__Map_LoadModelGuts = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x57"), "xxxx?xxxx?xxxxxxxxxx"); // BSP.
		p_CModelLoader__Map_IsValid       = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xDA\x48\x85\xD2\x0F\x84\x00\x00\x00\x00\x80\x3A\x00\x0F\x84\x00\x00\x00\x00\x4C\x8B\xCA"), "xxxxx????xxxxxxxx????xxxxx????xxx");
		p_GetSpriteInfo                   = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x4C\x8B\xBC\x24\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx????");
#endif
		p_BuildSpriteLoadName             = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x4D\x8B\xF1\x48\x8B\xF2"), "xxxx?xxxx?xxxx?xxxx?xxxxx????xxxxxx");

		CModelLoader__FindModel         = p_CModelLoader__FindModel.RCast<void* (*)(CModelLoader*, const char*)>();
		CModelLoader__LoadModel         = p_CModelLoader__LoadModel.RCast<void(*)(CModelLoader*, model_t*)>();
		CModelLoader__UnloadModel       = p_CModelLoader__UnloadModel.RCast<uint64_t(*)(CModelLoader*, model_t*)>();
		CModelLoader__Studio_LoadModel  = p_CModelLoader__Studio_LoadModel.RCast<void* (*)(CModelLoader*)>();
		CModelLoader__Map_LoadModelGuts = p_CModelLoader__Map_LoadModelGuts.RCast<uint64_t(*)(CModelLoader*, model_t* mod)>();
		CModelLoader__Map_IsValid       = p_CModelLoader__Map_IsValid.RCast<bool(*)(CModelLoader*, const char*)>();
		GetSpriteInfo                   = p_GetSpriteInfo.RCast<void* (*)(const char*, bool, bool, int&, int&, int&, void*)>();
		BuildSpriteLoadName             = p_BuildSpriteLoadName.RCast<void* (*)(const char*, char*, int, bool&, bool&)>();
	}
	virtual void GetVar(void) const
	{
		g_pModelLoader = g_GameDll.FindPatternSIMD(
			reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x53\x55\x56\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00"),
			"xxxx?xxxxxxxxxxxxxx????").FindPatternSelf("48 ?? 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(3, 7).RCast<CModelLoader*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VModelLoader);
