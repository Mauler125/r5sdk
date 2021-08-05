#include "pch.h"
#include "opcptc.h"

/*-----------------------------------------------------------------------------
 * _opcptc.cpp
 *-----------------------------------------------------------------------------*/

void InstallOpcodes() /* .TEXT */
{
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Prevent OriginSDK from initializing
	//Origin_Init.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 });
	//Origin_SetState.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Allow games to be loaded without the optional texture streaming file
	dst002.Offset(0x8E5).Patch({ 0xEB, 0x19 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect command from crashing by invalid call to UI function
	dst004.Offset(0x1D6).Patch({ 0xEB, 0x27 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect localhost from being executed after listenserver init
	//Host_NewGame.Offset(0x637).Patch({ 0xE9, 0xC1, 0x00, 0x00, 0x00});
	//-------------------------------------------------------------------------
	// JA  --> JMP | Disable server-side verification for duplicate accounts on the server
	CServer_Auth.Offset(0x284).Patch({ 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server
	FairFight_Init.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });
}
