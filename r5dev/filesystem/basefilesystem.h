#pragma once
#include "filesystem/filesystem.h"

class CBaseFileSystem
{
public:
	int Read(void* pOutput, int nSize, FileHandle_t hFile);
	FileHandle_t Open(const char* pFileName, const char* pOptions, const char* pPathID, int64_t unknown);
	void Close(FileHandle_t file);
	bool FileExists(const char* pFileName, const char* pPathID);
	static void Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);
	static FileHandle_t ReadFromVPK(CBaseFileSystem* pVpk, std::int64_t* pResults, char* pszFilePath);
	static bool ReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults);
};

/* ==== CBASEFILESYSTEM ================================================================================================================================================= */
inline CMemory p_CBaseFileSystem_Warning;
inline auto CBaseFileSystem_Warning = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem* thisptr, FileWarningLevel_t level, const char* fmt, ...)>();

inline CMemory p_CBaseFileSystem_LoadFromVPK;
inline auto CBaseFileSystem_LoadFromVPK = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem* thisptr, void* pResults, char* pszAssetName)>();

inline CMemory p_CBaseFileSystem_LoadFromCache;
inline auto CBaseFileSystem_LoadFromCache = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem* thisptr, char* pszAssetName, void* pResults)>();

inline CBaseFileSystem* g_pFileSystem = nullptr;

///////////////////////////////////////////////////////////////////////////////
void CBaseFileSystem_Attach();
void CBaseFileSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
class VBaseFileSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CBaseFileSystem::Warning             : {:#18x} |\n", p_CBaseFileSystem_Warning.GetPtr());
		spdlog::debug("| FUN: CBaseFileSystem::LoadFromVPK         : {:#18x} |\n", p_CBaseFileSystem_LoadFromVPK.GetPtr());
		spdlog::debug("| FUN: CBaseFileSystem::LoadFromCache       : {:#18x} |\n", p_CBaseFileSystem_LoadFromCache.GetPtr());
		spdlog::debug("| VAR: g_pFileSystem                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFileSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CBaseFileSystem_Warning       = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48"), "xxxxxx??????????x");
		p_CBaseFileSystem_LoadFromVPK   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xC0\x4C\x8D\x8C\x24\x00\x00\x00\x00"), "xxxx?xxxx????xxxxxxx????");
		p_CBaseFileSystem_LoadFromCache = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x49\x8B\xD8"), "xxxxx????xx?????xxx");

		CBaseFileSystem_Warning       = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem*, FileWarningLevel_t, const char*, ...)>(); /*4C 89 4C 24 20 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48*/
		CBaseFileSystem_LoadFromVPK   = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem*, void*, char*)>(); /*48 89 5C 24 ? 57 48 81 EC ? ? ? ? 49 8B C0 4C 8D 8C 24 ? ? ? ?*/
		CBaseFileSystem_LoadFromCache = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem*, char*, void*)>(); /*40 53 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 49 8B D8*/
	}
	virtual void GetVar(void) const
	{
		g_pFileSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxxxx????xxx????")
			.Offset(0x20).FindPatternSelf("48 89 05", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseFileSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VBaseFileSystem);
