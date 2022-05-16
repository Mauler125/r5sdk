#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "vpklib/packedstore.h"

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

class CFileSystem_Stdio
{
public:
	void AddSearchPath(const char* pPath, const char* pathID, SearchPathAdd_t addType);
	bool ReadFromCache(const char* pPath, void* pResult);
	VPKData_t* MountVPK(const char* vpkPath);
};
extern CFileSystem_Stdio* g_pFileSystem_Stdio;

///////////////////////////////////////////////////////////////////////////////
class VFileSystem_Stdio : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pFileSystem_Stdio                  : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pFileSystem_Stdio));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pFileSystem_Stdio = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x45\x33\xC0\x48\x83\xC1\x08\x48\x8B\x01"),
			"xxx????xxxxxxxxxx").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CFileSystem_Stdio*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VFileSystem_Stdio);
#endif // !FILESYSTEM_H
