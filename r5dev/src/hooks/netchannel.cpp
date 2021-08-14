#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	NetChan_ShutDown originalNetChanShutDown = nullptr;
}

void Hooks::NetChanShutdown(void* rcx, const char* reason, unsigned __int8 unk1, char unk2)
{
	reinterpret_cast<void(*)()>(addr_downloadPlaylists_Callback)(); // Re-Load playlist from disk after getting dropped or disconnecting off a server.
	originalNetChanShutDown(rcx, reason, unk1, unk2);
}