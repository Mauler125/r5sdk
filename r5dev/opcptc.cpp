#include "windows.h"
#include "opcptc.h"

/*-----------------------------------------------------------------------------
 * opcptc.cpp
 *-----------------------------------------------------------------------------*/

void InstallOpcodes()
{
	HANDLE GameProcess = GetCurrentProcess();
	// JNZ --> JMP | Prevent OriginSDK from initializing on the client
	//WriteProcessMemory(GetCurrentProcess(), LPVOID(dst000 + 0x0B), "\xE9\x63\x02\x00\x00\x00", 6, NULL);

	// JE  --> NOP | Allow execution of map commands even if the OriginSDK is not running
	WriteProcessMemory(GameProcess, LPVOID(dst001 + 0x2B), "\x90\x90\x90\x90\x90\x90", 6, NULL);
	WriteProcessMemory(GameProcess, LPVOID(dst001 + 0x39), "\x90\x90\x90\x90\x90\x90", 6, NULL);
	WriteProcessMemory(GameProcess, LPVOID(dst001 + 0x46), "\x90\x90\x90\x90\x90\x90", 6, NULL);

	// JL  --> NOP | Enable clientcommand callbacks without persistent player data
	WriteProcessMemory(GameProcess, LPVOID(dst002 + 0x76), "\x90\x90", 2, NULL);

	// JA  --> JMP | Disable client-side verification for duplicate accounts on the server
	WriteProcessMemory(GameProcess, LPVOID(dst003 + 0x269), "\xEB\x39", 2, NULL);

	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server
	WriteProcessMemory(GameProcess, LPVOID(dst004 + 0x72), "\xE9\xE4\x00\x00\x00\x00", 6, NULL);
}
