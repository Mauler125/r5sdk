#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CMemory p_Sys_Error = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x08\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x55\x41\x54\x41\x56\xB8\x58\x10\x00\x00\xE8"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto Sys_Error = p_Sys_Error.RCast<void (*)(char* fmt, ...)>(); /*48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 55 41 54 41 56 B8 58 10 00 00 E8*/

inline CMemory p_Warning = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\x48\x83\xEC\x28\x4C\x8D\x44\x24\x00\xE8\x00\x00\x00\x00\x48\x83\xC4\x28\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x8B\x05\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxx?x????xxxxxxxxxxxxxxxxxxxxxxx?xxxx?xxxx?xx????");
inline auto Sys_Warning = p_Warning.RCast<void* (*)(int, char* fmt, ...)>(); /*48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 48 83 EC 28 4C 8D 44 24 ? E8 ? ? ? ? 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 8B 05 ? ? ? ?*/

inline CMemory p_Sys_LoadAssetHelper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x18\x41\x56\x48\x83\xEC\x40\x33"), "xxxxxxxxxxxxxxxxx");
inline void*(*Sys_LoadAssetHelper)(const CHAR* lpFileName, std::int64_t a2, LARGE_INTEGER* a3) = (void*(*)(const CHAR*, std::int64_t, LARGE_INTEGER*))p_Sys_LoadAssetHelper.GetPtr();/*48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 40 33*/

inline CMemory p_Con_NPrintf = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\xC3"), "xxxx?xxxx?xxxx?xxxx?x");
inline auto Con_NPrintf = p_Con_NPrintf.RCast<void (*)(int pos, const char* fmt, ...)>(); /*48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? C3*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x85\xC0\x75\x0C\xE8\x16"), "xxxxxxxxx????xxxxxxxxxx");
inline auto MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>(); /*40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 75 0C E8 16*/
#elif defined (GAMEDLL_S2)
inline CMemory p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x85\xC0\x75\x0C\xE8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x4C\x8B\x00\x48\x8B\xD3\x48\x8B\xC8\x48\x83\xC4\x20\x5B\x49\xFF\x60\x08"), "xxxxxxxxx????xxxxxxxxx????xxx????xxxxxxxxxxxxxxxxxx");
inline auto MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>(); /*40 53 48 83 EC 20 48 8B 05 ? ? ? ? 48 8B D9 48 85 C0 75 0C E8 ? ? ? ? 48 89 05 ? ? ? ? 4C 8B 00 48 8B D3 48 8B C8 48 83 C4 20 5B 49 FF 60 08 */
#elif defined (GAMEDLL_S3)
inline CMemory p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x6B\x83\x25\x0D\x48\x8B\xD9"), "xxxxxxxxxxxxxxxx");
inline auto MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>(); /*40 53 48 83 EC 20 48 8B 05 6B 83 25 0D 48 8B D9*/
#endif
	/* ==== ------- ========================================================================================================================================================= */

enum class eDLL_T : int
{
	SERVER = 0, // Game DLL
	CLIENT = 1, // Game DLL
	UI     = 2, // Game DLL
	ENGINE = 3, // Wrapper
	FS     = 4, // File System
	RTECH  = 5, // RTech API
	MS     = 6, // Material System
	NETCON = 7, // Net Console
	NONE   = 8
};

const string sDLL_T[9] = 
{
	"Native(S):",
	"Native(C):",
	"Native(U):",
	"Native(E):",
	"Native(F):",
	"Native(R):",
	"Native(M):",
	"Netcon(X):",
	""
};

const static string sANSI_DLL_T[9] = 
{
	"\033[38;2;059;120;218mNative(S):",
	"\033[38;2;118;118;118mNative(C):",
	"\033[38;2;151;090;118mNative(U):",
	"\033[38;2;204;204;204mNative(E):",
	"\033[38;2;097;214;214mNative(F):",
	"\033[38;2;092;181;089mNative(R):",
	"\033[38;2;192;105;173mNative(M):",
	"\033[38;2;204;204;204mNetcon(X):",
	""
};

///////////////////////////////////////////////////////////////////////////////
void HSys_Error(char* fmt, ...);
void DevMsg(eDLL_T idx, const char* fmt, ...);
void Warning(eDLL_T idx, const char* fmt, ...);
void Error(eDLL_T idx, const char* fmt, ...);

void SysUtils_Attach();
void SysUtils_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSys_Utils : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Sys_Error                            : 0x" << std::hex << std::uppercase << p_Sys_Error.GetPtr()           << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Sys_Warning                          : 0x" << std::hex << std::uppercase << p_Warning.GetPtr()             << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Sys_LoadAssetHelper                  : 0x" << std::hex << std::uppercase << p_Sys_LoadAssetHelper.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Con_NPrintf                          : 0x" << std::hex << std::uppercase << p_Con_NPrintf.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: MemAlloc_Wrapper                     : 0x" << std::hex << std::uppercase << p_MemAlloc_Wrapper.GetPtr()    << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSys_Utils);
