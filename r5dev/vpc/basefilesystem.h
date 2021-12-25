#pragma once

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
}

///////////////////////////////////////////////////////////////////////////////
void CBaseFileSystem_Attach();
void CBaseFileSystem_Detach();

///////////////////////////////////////////////////////////////////////////////
class HBaseFileSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CBaseFileSystem::Warning             : 0x" << std::hex << std::uppercase << p_CBaseFileSystem_Warning.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseFileSystem);
