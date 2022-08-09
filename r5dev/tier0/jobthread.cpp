
#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "tier0/jobthread.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void* HJT_HelpWithAnything(bool bShouldLoadPak)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	static void* retaddr = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
		"\x48\x8B\xC4\x56\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x00"), 
		"xxxxxxxxxxx????xxxx????").Offset(0x400).FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN).RCast<void*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	static void* retaddr = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
		"\x48\x8B\xC4\x00\x41\x54\x41\x55\x48\x81\xEC\x70\x04\x00\x00\xF2\x0F\x10\x05\x00\x00\x00\x0B"), 
		"xxx?xxxxxxxxxxxxxxx???x").Offset(0x4A0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN).RCast<void*>();
#endif
	void* results = JT_HelpWithAnything(bShouldLoadPak);

	if (retaddr != _ReturnAddress()) // Check if this is called after 'PakFile_Init()'.
	{
		return results;
	}
	// Do stuff here after 'PakFile_Init()'.
	return results;
}

void JT_Attach()
{
	//DetourAttach((LPVOID*)&JT_HelpWithAnything, &HJT_HelpWithAnything);
}

void JT_Detach()
{
	//DetourDetach((LPVOID*)&JT_HelpWithAnything, &HJT_HelpWithAnything);
}
