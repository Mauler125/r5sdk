#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	LoadPlaylistFn originalLoadPlaylist = nullptr;
}

bool Hooks::LoadPlaylist(const char* playlist)
{
	memset(addr_MapVPKCache, 0, 0x40); // Bye bye vpk cache, you only make us crash >:(.

    CHAR playlistPath[] = "\x77\x27\x35\x2b\x2c\x6c\x2b\x2c\x2b";
    PCHAR curr = playlistPath;
    while (*curr) {
        *curr ^= 'B';
        ++curr;
    }

    if (FileExists(playlistPath))
    {
        std::uint8_t verifyPlaylistIntegrity[] = // Very hacky way for alternative inline assembly for x64..
        {
            0x48, 0x8B, 0x45, 0x58,       // mov rcx, playlist
            0xC7, 0x00, 0x00, 0x00, 0x00, // test playlist, playlist
            0x00
        };
        void* verifyPlaylistIntergrityFn = nullptr;
        VirtualAlloc(verifyPlaylistIntergrityFn, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        memcpy(&verifyPlaylistIntergrityFn, (const void*)verifyPlaylistIntegrity, 9);
        reinterpret_cast<void(*)()>(verifyPlaylistIntergrityFn)();
    }

	return originalLoadPlaylist(playlist); // Parse playlist like normally..
}