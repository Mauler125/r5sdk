#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	LoadPlaylistFn originalLoadPlaylist = nullptr;
}

bool Hooks::LoadPlaylist(const char* playlist)
{
	memset(addr_MapVPKCache, 0, 0x40); // Bye bye vpk cache, you only make us crash >:(.
	return originalLoadPlaylist(playlist); // Parse playlist like normally..
}