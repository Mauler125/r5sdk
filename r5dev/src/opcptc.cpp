#include "pch.h"
#include "opcptc.h"

/*-----------------------------------------------------------------------------
 * _opcptc.cpp
 *-----------------------------------------------------------------------------*/

void InstallOpcodes() /* .TEXT */
{
	spdlog::debug("Patching the game executeable..\n");
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
	//-------------------------------------------------------------------------
	// CALL --> NOP | Prevent squirrel compiler errors from calling Error
	Squirrel_CompileError.Offset(0x12C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// CALL --> NOP | Prevent random netchan encryption key from being overriden by default key
	NetChan_EncKey_DefaultAssign.Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// INLINE CALL --> VTABLE CALL | Call LockCursor VTable class function instead of doing it inlined.
	//-------------------------------------------------------------------------
	// .text:0000000140548E2C 80 3D 5C 2E 1D 01 00     cmp     cs:byte_14171BC8F, 0
	// .text:0000000140548E33 48 8B 0D 16 25 EC 0C     mov     rcx, cs:g_InputStackSystem
	// .text:0000000140548E3A 48 8B 97 18 01 00 00     mov     rdx, [rdi+118h]
	// .text:0000000140548E41 C6 05 91 7B EC 0C 01     mov     cs:byte_14D4109D9, 1
	// .text:0000000140548E48 48 8B 01                 mov     rax, [rcx]
	// .text:0000000140548E4B 74 10                    jz      short loc_140548E5D
	// .text:0000000140548E4D 4C 8B 05 8C 7B EC 0C     mov     r8, cs:qword_14D4109E0
	// .text:0000000140548E54 48 83 C4 30              add     rsp, 30h
	// .text:0000000140548E58 5F                       pop     rdi
	// .text:0000000140548E59 48 FF 60 60              jmp     qword ptr[rax+60h]
	//-------------------------------------------------------------------------
	// TURNS INTO:
	//-------------------------------------------------------------------------
	// .text:0000000140548E2C 48 8B 07                 mov     rax, [rdi]
	// .text:0000000140548E2F 48 89 F9                 mov     rcx, rdi
	// .text:0000000140548E32 FF 90 90 02 00 00        call    qword ptr[rax+290h]
	// .text:0000000140548E38 EB 2F                    jmp     short loc_140548E69
	//-------------------------------------------------------------------------
	MemoryAddress(0x140548E2C).Patch({ 0x48, 0x8B, 0x07, 0x48, 0x89, 0xF9, 0xFF, 0x90, 0x90, 0x02, 0x00, 0x00, 0xEB, 0x2F });
}
