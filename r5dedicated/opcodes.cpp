#include "pch.h"
#include "hooks.h"
#include "enums.h"
#include "opcodes.h"
#include "gameclasses.h"

/*-----------------------------------------------------------------------------
 * _opcodes.cpp
 *-----------------------------------------------------------------------------*/

void DisableRenderer()
{
	//-------------------------------------------------------------------------
	// FUN --> RET | Called from CEngineClient and CEngineVGUI (Init()?).
	r0.Patch({ 0xC3 }); // This patch is likely not required if client.dll isn't initialized.
	//-------------------------------------------------------------------------
	// FUN --> RET | Skip ShaderSetup(). CShaderGlue.
	r1.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Skip Matsync. Called from CMaterialSystem.
	r2.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Matsys mode init (CMaterialSystem).
	r3.Offset(0x22).Patch({ 0xEB, 0x66 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Clear render buffer? Called from CMatRenderContext and CTexture.
	r4.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Heavy render stuff. Called from CMatRenderContext.
	r5.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Set shader resource.
	r6.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Begin.
	r7.Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// FUN --> RET | End.
	r8.Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });
}

void DisableClient()
{
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Prevent EbisuSDK from initializing on the engine and server.
	Origin_Init.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 });
	Origin_SetState.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip CreateGameWindow initialization code.
	CreateGameWindow.Offset(0x3DD).Patch({ 0xEB, 0x6D });
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Skip CreateGameWindow validation code.
	CreateGameWindow.Offset(0x44C).Patch({ 0xEB, 0x49 });
	//-------------------------------------------------------------------------
	// PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized.
	c1.Patch({ 0x30, 0xC0, 0xC3 });
	//-------------------------------------------------------------------------
	// PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized.
	c1.Patch({ 0x30, 0xC0, 0xC3 });
	//-------------------------------------------------------------------------
	// JNE --> NOP | TODO: NOP 'particle_script' instead.
	c2.Offset(0x23C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// MOV --> NOP | TODO: NOP 'particle_script' instead.
	c2.Offset(0x2BD).Patch({ 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// MOV --> NOP | TODO: NOP 'highlight_system' instead.
	c3.Offset(0xA9).Patch({ 0x90, 0x90, 0x90, 0x90 });
}

void DisableVGUI()
{
	//-------------------------------------------------------------------------
	// CMP --> XOR | Skip VGUI initialization jumptable.
	v0.Patch({ 0x48, 0x33, 0xC0, 0xC3, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Skip call to VGUI loadscreen func.
	SCR_BeginLoadingPlaque.Offset(0x427).Patch({ 0xEB, 0x09 });
}

void Hooks::DedicatedPatch()
{
	//Sleep(10000);
	// for future reference 14171A9B4 - matsys mode

	*(uintptr_t*)0x14D415040 = 0x1417304E8;
	*(uintptr_t*)0x14B37C3C0 = 0x141F10CA0;

	*(uintptr_t*)0x14B3800D7 = 0x1; // bDedicated

	DisableRenderer();
	DisableClient();
	DisableVGUI();

	//-------------------------------------------------------------------------
	// CAL --> NOP | HLClient call inside eng->frame.
	addr_CEngine_Frame.Offset(0x410).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server.
	// TODO: fix and re-enable this.
	FairFight_Init.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Take dedicated initialization routine instead.
	s0.Offset(0x19).Patch({ 0xEB, 0x6E });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip client.dll Init_PostVideo() validation code.
	s0.Offset(0x609).Patch({ 0xEB, 0x2B });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Skip client.dll Init_PostVideo() validation code.
	s0.Offset(0x621).Patch({ 0xEB, 0x0C });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip NULL call as client is never initialized.
	s0.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.
	s0.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.
	//s1.Offset(0x1C6).Patch({ 0xE9, 0xAD, 0x11, 0x00, 0x00 }); // <-- this one was only used to debug.
	//-------------------------------------------------------------------------
	// JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.
	s1.Offset(0x1010).Patch({ 0xEB, 0x14 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP NULL call as client is never initialized.
	s1.Offset(0x1023).Patch({ 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JS  --> JMP | Skip ListenServer HeartBeat.
	s2.Offset(0xF).Patch({ 0xE9, 0x22, 0x01, 0x00, 0x00 });

	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to UI texture asset preloading.
	e0.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JNE --> JNP | Skip client.dll library initialization.
	e0.Offset(0xA7D).Patch({ 0xE9, 0xF0, 0x01, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> NOP | Skip settings field loading for client texture assets.
	// TODO: this is also used by server.dll library.
	e1.Offset(0x213).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });

	//-------------------------------------------------------------------------
	// ??? 1403DFC30 = 0x94490 ??? // an expensive stuff that wasted many CPU cycles, this one seems to be the best candidate to return
}

// TEST
void SetCHostState()
{
	static std::string ServerMap = std::string();
	ServerMap = "mp_rr_canyonlands_64k_x_64k";
	strncpy_s(GameGlobals::HostState->m_levelName, ServerMap.c_str(), 64); // Copy new map into hoststate levelname. 64 is size of m_levelname.
	GameGlobals::HostState->m_iNextState = HostStates_t::HS_NEW_GAME; // Force CHostState::FrameUpdate to start a server.
}