#include "pch.h"
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
	//dst000.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 });
	//dst001.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00 }); 
	//-------------------------------------------------------------------------
	// JNE --> JMP | Allow games to be loaded without the optional texture streaming file
	dst002.Offset(0x8E5).Patch({ 0xEB, 0x19 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect command from crashing by invalid call to UI function
	dst004.Offset(0x1D6).Patch({ 0xEB, 0x27 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect localhost from being executed after listenserver init
	//WriteProcessMemory(GameProcess, LPVOID(Host_NewGame + 0x637), "\xE9\xC1\x00\x00\x00", 5, NULL);
	//Host_NewGame.Offset(0x637).Patch({ 0xE9, 0xC1, 0x00, 0x00, 0x00});
	//-------------------------------------------------------------------------
	// JA  --> JMP | Disable server-side verification for duplicate accounts on the server
	dst006.Offset(0x284).Patch({ 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server
	dst007.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });
}
