#pragma once
typedef void* FileHandle_t;

enum class SearchPathAdd_t : int
{
	PATH_ADD_TO_HEAD,         // First path searched
	PATH_ADD_TO_TAIL,         // Last path searched
	PATH_ADD_TO_TAIL_ATINDEX, // First path searched
};

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

class CBaseFileSystem
{
public:
	int Read(void* pOutput, int nSize, FileHandle_t hFile);
	FileHandle_t Open(const char* pFileName, const char* pOptions, const char* pPathID, int64_t unknown);
	void Close(FileHandle_t file);
	bool FileExists(const char* pFileName, const char* pPathID);
	static void Warning(CBaseFileSystem* pFileSystem, FileWarningLevel_t level, const char* fmt, ...);
	static FileHandle_t ReadFromVPK(CBaseFileSystem* pVpk, FileHandle_t pResults, char* pszFilePath);
	static bool ReadFromCache(CBaseFileSystem* pFileSystem, char* pszFilePath, void* pResults);
	static void AddSearchPath(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID, SearchPathAdd_t addType);
	static bool RemoveSearchPath(CBaseFileSystem* pFileSystem, const char* pPath, const char* pPathID);
};

/* ==== CBASEFILESYSTEM ================================================================================================================================================= */
inline CMemory p_CBaseFileSystem_Warning;
inline auto CBaseFileSystem_Warning = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem* thisptr, FileWarningLevel_t level, const char* fmt, ...)>();

inline CMemory p_CBaseFileSystem_LoadFromVPK;
inline auto CBaseFileSystem_LoadFromVPK = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem* thisptr, FileHandle_t pResults, char* pszAssetName)>();

inline CMemory p_CBaseFileSystem_LoadFromCache;
inline auto CBaseFileSystem_LoadFromCache = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem* thisptr, char* pszAssetName, void* pResults)>();

inline CMemory p_CBaseFileSystem_AddSearchPath;
inline auto CBaseFileSystem_AddSearchPath = p_CBaseFileSystem_AddSearchPath.RCast<void(*)(CBaseFileSystem* thisptr, const char* pPath, const char* pPathID, SearchPathAdd_t addType)>();

inline CMemory p_CBaseFileSystem_RemoveSearchPath;
inline auto CBaseFileSystem_RemoveSearchPath = p_CBaseFileSystem_RemoveSearchPath.RCast<bool(*)(CBaseFileSystem* thisptr, const char* pPath, const char* pPathID)>();

extern CBaseFileSystem* g_pFileSystem;

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
		spdlog::debug("| FUN: CBaseFileSystem::AddSearchPath       : {:#18x} |\n", p_CBaseFileSystem_AddSearchPath.GetPtr());
		spdlog::debug("| FUN: CBaseFileSystem::RemoveSearchPath    : {:#18x} |\n", p_CBaseFileSystem_RemoveSearchPath.GetPtr());
		spdlog::debug("| VAR: g_pFileSystem                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFileSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CBaseFileSystem_Warning          = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48"), "xxxxxx??????????x");
		p_CBaseFileSystem_LoadFromVPK      = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x49\x8B\xC0\x4C\x8D\x8C\x24\x00\x00\x00\x00"), "xxxx?xxxx????xxxxxxx????");
		p_CBaseFileSystem_LoadFromCache    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x49\x8B\xD8"), "xxxxx????xx?????xxx");
		p_CBaseFileSystem_AddSearchPath    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x4C\x24\x00\x48\x89\x4C\x24\x00\x55\x57"), "xxxx?xxxx?xx");
		p_CBaseFileSystem_RemoveSearchPath = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x55\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xC6\x44\x24\x00\x00"), "xxxxxxxxxxxxxx????xxx??");

		CBaseFileSystem_Warning          = p_CBaseFileSystem_Warning.RCast<void(*)(CBaseFileSystem*, FileWarningLevel_t, const char*, ...)>();            /*4C 89 4C 24 20 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48*/
		CBaseFileSystem_LoadFromVPK      = p_CBaseFileSystem_LoadFromVPK.RCast<FileHandle_t(*)(CBaseFileSystem*, FileHandle_t, char*)>();                 /*48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 49 8B C0 4C 8D 8C 24 ?? ?? ?? ??*/
		CBaseFileSystem_LoadFromCache    = p_CBaseFileSystem_LoadFromCache.RCast<bool(*)(CBaseFileSystem*, char*, void*)>();                              /*40 53 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 49 8B D8*/
		CBaseFileSystem_AddSearchPath    = p_CBaseFileSystem_AddSearchPath.RCast<void(*)(CBaseFileSystem*, const char*, const char*, SearchPathAdd_t)>(); /*44 89 4C 24 ?? 48 89 4C 24 ?? 55 57*/
		CBaseFileSystem_RemoveSearchPath = p_CBaseFileSystem_RemoveSearchPath.RCast<bool(*)(CBaseFileSystem*, const char*, const char*)>();               /*40 53 55 56 57 41 54 41 56 41 57 48 81 EC ?? ?? ?? ?? C6 44 24 ?? ??*/
	}
	virtual void GetVar(void) const
	{
		g_pFileSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\xE9\x00\x00\x00\x00"),
			"xxx????xxx????xxx????xxx????xxx????x????").FindPattern("48 89", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CBaseFileSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VBaseFileSystem);
