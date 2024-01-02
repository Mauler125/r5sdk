
#include "engine/host_cmd.h"
#include "tier0/jobthread.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void* HJT_HelpWithAnything(bool bShouldLoadPak)
{
	// !! Uncomment if needed !!
	// !! Consider initializing this in idetour iface if enabled !!
	//static void* const retaddr = g_GameDll.FindPatternSIMD("48 8B C4 ?? 41 54 41 55 48 81 EC 70 04 ?? ?? F2 0F 10 05 ?? ?? ?? 0B")
	//	.Offset(0x4A0).FindPatternSelf("48 8B ?? ?? ?? ?? 01", CMemory::Direction::DOWN).RCast<void*>();

	void* const results = JT_HelpWithAnything(bShouldLoadPak);

	//if (retaddr != _ReturnAddress()) // Check if this is called after 'PakFile_Init()'.
	//{
	//	return results;
	//}
	// Do stuff here after 'PakFile_Init()'.
	return results;
}

void VJobThread::Detour(const bool bAttach) const
{
	//DetourSetup(&JT_HelpWithAnything, &HJT_HelpWithAnything, bAttach);
}
