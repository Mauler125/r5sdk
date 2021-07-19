#include "windows.h"
#include "opcptc.h"

/*-----------------------------------------------------------------------------
 * _opcptc.cpp
 *-----------------------------------------------------------------------------*/

void InstallOpcodes() /* .TEXT */
{
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Prevent OriginSDK from initializing on the client
	//WriteProcessMemory(GameProcess, LPVOID(dst000 + 0x0B), "\xE9\x63\x02\x00\x00\x00", 6, NULL);
	//WriteProcessMemory(GameProcess, LPVOID(dst001 + 0x0E), "\xE9\xCB\x03\x00\x00", 5, NULL);
	//-------------------------------------------------------------------------
	// JNE --> JMP | Allow games to be loaded without the optional texture streaming file
	WriteProcessMemory(GameProcess, LPVOID(dst002 + 0x8E5), "\xEB\x19", 2, NULL);
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect command from crashing by invalid call to UI function
	WriteProcessMemory(GameProcess, LPVOID(dst004 + 0x1D6), "\xEB\x27", 2, NULL);
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect localhost from being executed after listenserver init
	//WriteProcessMemory(GameProcess, LPVOID(Host_NewGame + 0x637), "\xE9\xC1\x00\x00\x00", 5, NULL);
	//-------------------------------------------------------------------------
	// JA  --> JMP | Disable server-side verification for duplicate accounts on the server
	WriteProcessMemory(GameProcess, LPVOID(dst006 + 0x284), "\x90\x90", 2, NULL);
	//-------------------------------------------------------------------------
	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server
	WriteProcessMemory(GameProcess, LPVOID(dst007 + 0x61), "\xE9\xED\x00\x00\x00\x00", 6, NULL);
}
