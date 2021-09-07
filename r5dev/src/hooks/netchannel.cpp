#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	NetChan_ShutDown originalNetChan_ShutDown = nullptr;
}

void Hooks::NetChan_Shutdown(void* rcx, const char* reason, unsigned __int8 unk1, char unk2)
{
	addr_downloadPlaylists_Callback(); // Re-Load playlist from disk after getting dropped or disconnecting off a server.
	originalNetChan_ShutDown(rcx, reason, unk1, unk2);
}