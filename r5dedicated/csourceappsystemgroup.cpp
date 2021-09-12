#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	CSourceAppSystemGroup_CreateFn originalCSourceAppSystemGroup_Create = nullptr;
}
auto g_bIsDedicated = (uint8_t*)0x162C61208;

//-----------------------------------------------------------------------------
// Purpose: sets 'EbisuSDK' globals required in certain engine callbacks.
//-----------------------------------------------------------------------------
void HEbisuSDK_Init()
{
	auto ofs000 = (uint8_t*)0x1634F1690;
	auto ofs001 = (uint8_t*)0x1634F16B0;
	auto ofs002 = (uint8_t*)0x1634F1695;
	auto ofs003 = (uint8_t*)0x1634F30D8;
	auto ofs004 = (uint8_t*)0x1634F31D8;

	*(char*)(ofs000) = (char)0x1; // <-- | 1st EbisuSDK boolean to be checked.
	*(char*)(ofs001) = (char)0x1; // <-- | 2nd EbisuSDK boolean to be checked.
	*(char*)(ofs002) = (char)0x1; // <-- | 3rd EbisuSDK boolean to be checked.
	*(char*)(ofs003) = (char)0x1; // <-- | Gets tested on listenserver for certain ConCommands.
	*(char*)(ofs004) = (char)0x0; // <-- | TODO: enforces Necleus cvars when not equal to NULL.
}

//-----------------------------------------------------------------------------
// Purpose: hook 'SourceAppSystemGroup::Create' and set m_bIsDedicated to true.
//-----------------------------------------------------------------------------
char __fastcall Hooks::CSourceAppSystemGroup_Create(__int64 a1)
{
	*g_bIsDedicated = 1; // HAS TO BE HERE!!!
	HEbisuSDK_Init();
	return originalCSourceAppSystemGroup_Create(a1);
}