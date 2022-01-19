#pragma once

typedef void* FileHandle_t;

enum class FileWarningLevel_t : int
{
	FILESYSTEM_WARNING = -1,                        // A problem!
	FILESYSTEM_WARNING_QUIET = 0,                   // Don't print anything
	FILESYSTEM_WARNING_REPORTUNCLOSED,              // On shutdown, report names of files left unclosed
	FILESYSTEM_WARNING_REPORTUSAGE,                 // Report number of times a file was opened, closed
	FILESYSTEM_WARNING_REPORTALLACCESSES,           // Report all open/close events to console ( !slow! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READ,      // Report all open/close/read events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE, // Report all open/close/read/write events to the console ( !slower! )
	FILESYSTEM_WARNING_REPORTALLACCESSES_ASYNC      // Report all open/close/read/write events and all async I/O file events to the console ( !slower(est)! )
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
}

///////////////////////////////////////////////////////////////////////////////
void CBaseFileSystem_Attach();
void CBaseFileSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
class HBaseFileSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CBaseFileSystem::Warning             : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_Warning.GetPtr()     << std::setw(npad)   << " |" << std::endl;
		std::cout << "| FUN: CBaseFileSystem::LoadFromVPK         : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_LoadFromVPK.GetPtr() << std::setw(npad)   << " |" << std::endl;
		std::cout << "| FUN: CBaseFileSystem::LoadFromCache       : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_LoadFromCache.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseFileSystem);
