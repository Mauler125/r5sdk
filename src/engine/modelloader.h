#pragma once
#include "engine/gl_model_private.h"
#include "public/bspfile.h"

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

class CMapLoadHelper
{
public:
	static void Constructor(CMapLoadHelper* helper, int lumpToLoad);

public:
	int m_nLumpSize;
	int m_nLumpOffset;
	int m_nLumpVersion;
	byte* m_pRawData;
	byte* m_pData;
	byte* m_pUncompressedData;
	int m_nUncompressedLumpSize;
	bool m_bUncompressedDataExternal;
	bool m_bExternal;
	bool m_bUnk;
	int m_nLumpID;
	char m_szLumpFilename[260];
};

inline void*(*CModelLoader__FindModel)(CModelLoader* loader, const char* pszModelName);
inline void(*CModelLoader__LoadModel)(CModelLoader* loader, model_t* model);
inline uint64_t(*CModelLoader__UnloadModel)(CModelLoader* loader, model_t* model);
inline void*(*CModelLoader__Studio_LoadModel)(CModelLoader* loader);
inline uint64_t(*CModelLoader__Map_LoadModelGuts)(CModelLoader* loader, model_t* model);
inline bool(*CModelLoader__Map_IsValid)(CModelLoader* loader, const char* pszMapName);
inline void(*CMapLoadHelper__CMapLoadHelper)(CMapLoadHelper * helper, int lumpToLoad);

inline void(*v_AddGameLump)(void);
inline void(*v_Map_LoadModel)(void);

inline void*(*v_GetSpriteInfo)(const char* pName, bool bIsAVI, bool bIsBIK, int& nWidth, int& nHeight, int& nFrameCount, void* a7);
inline void*(*v_BuildSpriteLoadName)(const char* pName, char* pOut, int outLen, bool& bIsAVI, bool& bIsBIK);

inline CModelLoader* g_pModelLoader;
inline FileHandle_t* s_MapFileHandle;
inline BSPHeader_t* s_MapHeader;
inline char* s_szMapPathName; /*size = 260*/

///////////////////////////////////////////////////////////////////////////////
class VModelLoader : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CModelLoader::FindModel", CModelLoader__FindModel);
		LogFunAdr("CModelLoader::LoadModel", CModelLoader__LoadModel);
		LogFunAdr("CModelLoader::UnloadModel", CModelLoader__UnloadModel);
		LogFunAdr("CModelLoader::Map_LoadModelGuts", CModelLoader__Map_LoadModelGuts);
		LogFunAdr("CModelLoader::Map_IsValid", CModelLoader__Map_IsValid);
		LogFunAdr("CModelLoader::Studio_LoadModel", CModelLoader__Studio_LoadModel);

		LogFunAdr("CMapLoadHelper::CMapLoadHelper", CMapLoadHelper__CMapLoadHelper);

		LogFunAdr("AddGameLump", v_AddGameLump);
		LogFunAdr("Map_LoadModel", v_Map_LoadModel);
		LogFunAdr("GetSpriteInfo", v_GetSpriteInfo);
		LogFunAdr("BuildSpriteLoadName", v_BuildSpriteLoadName);

		LogVarAdr("g_pModelLoader", g_pModelLoader);
		LogVarAdr("s_MapFileHandle", s_MapFileHandle);
		LogVarAdr("s_MapHeader", s_MapHeader);
		LogVarAdr("s_szMapPathName", s_szMapPathName);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 55 41 57 48 83 EC 48 80 3A 2A").GetPtr(CModelLoader__FindModel);
		g_GameDll.FindPatternSIMD("40 53 57 41 57 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??").GetPtr(CModelLoader__LoadModel);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 33 ED").GetPtr(CModelLoader__UnloadModel);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 56 57 41 54 41 57 48 81 EC ?? ?? ?? ??").GetPtr(CModelLoader__Studio_LoadModel);
		g_GameDll.FindPatternSIMD("48 89 54 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 57").GetPtr(CModelLoader__Map_LoadModelGuts); // BSP.
		g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 48 8B DA 48 85 D2 0F 84 ?? ?? ?? ?? 80 3A ?? 0F 84 ?? ?? ?? ?? 4C 8B CA").GetPtr(CModelLoader__Map_IsValid);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 30 4C 8B BC 24 ?? ?? ?? ??").GetPtr(v_GetSpriteInfo);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 4D 8B F1 48 8B F2").GetPtr(v_BuildSpriteLoadName);
		
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC 60").GetPtr(CMapLoadHelper__CMapLoadHelper);
		g_GameDll.FindPatternSIMD("40 ?? 57 48 83 EC 48 33 ?? 48 8D").GetPtr(v_AddGameLump);
		g_GameDll.FindPatternSIMD("48 83 EC 28 8B 05 ?? ?? ?? ?? FF C8").GetPtr(v_Map_LoadModel);
	}
	virtual void GetVar(void) const
	{
		g_pModelLoader = g_GameDll.FindPatternSIMD(
			"48 89 4C 24 ?? 53 55 56 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??").FindPatternSelf("48 ?? 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(3, 7).RCast<CModelLoader*>();

		s_MapFileHandle = CMemory(v_Map_LoadModel).FindPattern("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<FileHandle_t*>();
		s_MapHeader = CMemory(v_Map_LoadModel).FindPattern("48 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<BSPHeader_t*>();
		s_szMapPathName = CMemory(CMapLoadHelper__CMapLoadHelper).FindPattern("4C 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<char*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
