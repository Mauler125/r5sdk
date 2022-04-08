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

namespace
{
	/* ==== CBASEFILESYSTEM ================================================================================================================================================= */
	ADDRESS p_CBaseFileSystem_Warning = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x4C\x24\x20\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48", "xxxxxx??????????x");
	void (*CBaseFileSystem_Warning)(void* thisptr, FileWarningLevel_t level, const char* fmt, ...) = (void (*)(void*, FileWarningLevel_t, const char*, ...))p_CBaseFileSystem_Warning.GetPtr(); /*4C 89 4C 24 20 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48*/

	ADDRESS p_CBaseFileSystem_LoadFromVPK = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xC0\x4C\x8D\x8C\x24\x00\x00\x00\x00", "xxxx?xxxx????xxxxxxx????");
	FileHandle_t(*CBaseFileSystem_LoadFromVPK)(void* pVpk, std::int64_t* pResults, char* pszAssetName) = (FileHandle_t(*)(void*, std::int64_t*, char*))p_CBaseFileSystem_LoadFromVPK.GetPtr(); /*48 89 5C 24 ? 57 48 81 EC ? ? ? ? 49 8B C0 4C 8D 8C 24 ? ? ? ?*/

	ADDRESS p_CBaseFileSystem_LoadFromCache = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x49\x8B\xD8", "xxxxx????xx?????xxx");
	bool(*CBaseFileSystem_LoadFromCache)(void* pFileSystem, char* pszAssetName, void* pResults) = (bool(*)(void*, char*, void*))p_CBaseFileSystem_LoadFromCache.GetPtr(); /*40 53 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 49 8B D8*/

	CBaseFileSystem* g_pFileSystem = g_mGameDll.FindPatternSIMD((uint8_t*)"\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00", "xxxxx????xxx????")
		.Offset(0x20).FindPatternSelf("48 89 05", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseFileSystem*>();
}

///////////////////////////////////////////////////////////////////////////////
void CBaseFileSystem_Attach();
void CBaseFileSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
class HBaseFileSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CBaseFileSystem::Warning             : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_Warning.GetPtr()       << std::setw(npad)   << " |" << std::endl;
		std::cout << "| FUN: CBaseFileSystem::LoadFromVPK         : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_LoadFromVPK.GetPtr()   << std::setw(npad)   << " |" << std::endl;
		std::cout << "| FUN: CBaseFileSystem::LoadFromCache       : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_LoadFromCache.GetPtr() << std::setw(npad)   << " |" << std::endl;
		std::cout << "| VAR: g_pFileSystem                        : 0x" << std::hex << std::uppercase << g_pFileSystem                            << std::setw(0)      << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseFileSystem);
