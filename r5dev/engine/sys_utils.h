#pragma once
#include "tier0/basetypes.h"

namespace
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ADDRESS p_Sys_Error = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x4C\x24\x08\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x55\x41\x54\x41\x56\xB8\x58\x10\x00\x00\xE8", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	void (*Sys_Error)(char* fmt, ...) = (void (*)(char* fmt, ...))p_Sys_Error.GetPtr(); /*48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 55 41 54 41 56 B8 58 10 00 00 E8*/

	ADDRESS p_Sys_LoadAssetHelper = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x18\x41\x56\x48\x83\xEC\x40\x33", "xxxxxxxxxxxxxxxxx");
	void*(*Sys_LoadAssetHelper)(const CHAR* lpFileName, std::int64_t a2, LARGE_INTEGER* a3) = (void*(*)(const CHAR*, std::int64_t, LARGE_INTEGER*))p_Sys_LoadAssetHelper.GetPtr();/*48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 40 33*/
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
	ADDRESS p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x85\xC0\x75\x0C\xE8\x16", "xxxxxxxxx????xxxxxxxxxx");
	void* (*MemAlloc_Wrapper)(std::int64_t size) = (void* (*)(std::int64_t))p_MemAlloc_Wrapper.GetPtr(); /*40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 75 0C E8 16*/
#elif defined (GAMEDLL_S3)
	ADDRESS p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x6B\x83\x25\x0D\x48\x8B\xD9", "xxxxxxxxxxxxxxxx");
	void* (*MemAlloc_Wrapper)(std::int64_t size) = (void* (*)(std::int64_t))p_MemAlloc_Wrapper.GetPtr(); /*40 53 48 83 EC 20 48 8B 05 6B 83 25 0D 48 8B D9*/
#endif
	/* ==== ------- ========================================================================================================================================================= */
}

enum class eDLL_T : int
{
	SERVER = 0, // Game DLL
	CLIENT = 1, // Game DLL
	UI     = 2, // Game DLL
	ENGINE = 3, // Wrapper
	FS     = 4, // File System
	RTECH  = 5, // RTech API
	MS     = 6  // Material System
};

///////////////////////////////////////////////////////////////////////////////
void HSys_Error(char* fmt, ...);
void DevMsg(eDLL_T idx, const char* fmt, ...);

void SysUtils_Attach();
void SysUtils_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSys_Utils : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Sys_Error                            : 0x" << std::hex << std::uppercase << p_Sys_Error.GetPtr()           << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Sys_LoadAssetHelper                  : 0x" << std::hex << std::uppercase << p_Sys_LoadAssetHelper.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: MemAlloc_Wrapper                     : 0x" << std::hex << std::uppercase << p_MemAlloc_Wrapper.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Utils);
