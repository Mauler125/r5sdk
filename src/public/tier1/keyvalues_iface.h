#pragma once
#include "tier1/keyvalues.h"

//---------------------------------------------------------------------------------
// Purpose: Forward declarations
//---------------------------------------------------------------------------------
class KeyValues;
class CFileSystem_Stdio;
class IBaseFileSystem;
class CKeyValuesTokenReader;

/* ==== KEYVALUES ======================================================================================================================================================= */
inline void*(*KeyValues__FindKey)(KeyValues* thisptr, const char* pkeyName, bool bCreate);
inline KeyValues*(*KeyValues__ReadKeyValuesFile)(CFileSystem_Stdio* pFileSystem, const char* pFileName);
inline void(*KeyValues__RecursiveSaveToFile)(KeyValues* thisptr, IBaseFileSystem* pFileSystem, FileHandle_t pHandle, CUtlBuffer* pBuf, int nIndentLevel);
inline KeyValues*(*KeyValues__LoadFromFile)(KeyValues* thisptr, IBaseFileSystem* pFileSystem, const char* pszResourceName, const char* pszPathID, void* pfnEvaluateSymbolProc);

///////////////////////////////////////////////////////////////////////////////
extern KeyValues** g_pPlaylistKeyValues;

///////////////////////////////////////////////////////////////////////////////
class VKeyValues : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("KeyValues::FindKey", KeyValues__FindKey);
		LogFunAdr("KeyValues::ReadKeyValuesFile", KeyValues__ReadKeyValuesFile);
		LogFunAdr("KeyValues::RecursiveSaveToFile", KeyValues__RecursiveSaveToFile);
		LogFunAdr("KeyValues::LoadFromFile", KeyValues__LoadFromFile);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 56 57 41 57 48 81 EC ?? ?? ?? ?? 45").GetPtr(KeyValues__FindKey);
		g_GameDll.FindPatternSIMD("48 8B C4 55 53 57 41 54 48 8D 68 A1").GetPtr(KeyValues__ReadKeyValuesFile);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 4C 89 4C 24 ?? 48 89 4C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ??").GetPtr(KeyValues__LoadFromFile);
		g_GameDll.FindPatternSIMD("48 8B C4 53 ?? 57 41 55 41 ?? 48 83").GetPtr(KeyValues__RecursiveSaveToFile);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
