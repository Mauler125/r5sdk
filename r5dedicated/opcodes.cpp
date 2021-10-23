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
	//Sleep(2500);
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Prevent EbisuSDK from initializing on the engine and server.
	Origin_Init.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 });
	Origin_SetState.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip CreateGameWindow initialization code.
	//CreateGameWindow.Offset(0x3DD).Patch({ 0xEB, 0x6D });
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Skip CreateGameWindow validation code.
	CreateGameWindow.Offset(0x44C).Patch({ 0xEB, 0x49 });
	//-------------------------------------------------------------------------
	// PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized.
	c1.Offset(0x2C).Patch({ 0xE9, 0x9A, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> NOP | TODO: NOP 'particle_script' instead.
	c2.Offset(0x23C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// MOV --> NOP | TODO: NOP 'particle_script' instead.
	c2.Offset(0x2BD).Patch({ 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// MOV --> NOP | TODO: NOP 'highlight_system' instead.
	c3.Offset(0xA9).Patch({ 0x90, 0x90, 0x90, 0x90 });

	//-------------------------------------------------------------------------
	// FUN --> RET | 
	c4.Patch({ 0xC3 });
	c5.Patch({ 0xC3 });
	c7.Patch({ 0xC3 });

	//-------------------------------------------------------------------------
	// JE  --> JMP | Render?
	//gHost_Init_2.Offset(0x5D8).Patch({ 0xEB, 0x05 });

	//-------------------------------------------------------------------------
	// FUN --> RET | Disable particle effects precaching on the server.
	ParticleEffect_Init.Patch({ 0xC3 });

	//-------------------------------------------------------------------------
	// JNE --> JMP | 
	c6.Offset(0x23).Patch({ 0xEB, 0x23 });
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
	gHost_Init_1.Offset(0x19).Patch({ 0xEB, 0x6E });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x609).Patch({ 0xEB, 0x2B });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x621).Patch({ 0xEB, 0x0C });
	//-------------------------------------------------------------------------
	// JE  --> JMP | Skip NULL call as client is never initialized.
	gHost_Init_1.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.
	gHost_Init_1.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.
	//s1.Offset(0x1C6).Patch({ 0xE9, 0xAD, 0x11, 0x00, 0x00 }); // <-- this one was only used to debug.
	//-------------------------------------------------------------------------
	// JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.
	//s1.Offset(0x1010).Patch({ 0xEB, 0x14 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP NULL call as client is never initialized.
	s1.Offset(0x1023).Patch({ 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JS  --> JMP | Skip ListenServer HeartBeat.
	s2.Offset(0xF).Patch({ 0xE9, 0x22, 0x01, 0x00, 0x00 });

	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to UI texture asset preloading.
	gHost_Init_0.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JNE --> JNP | Skip client.dll library initialization.
	gHost_Init_0.Offset(0xA7D).Patch({ 0xE9, 0xF0, 0x01, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> NOP | Skip settings field loading for client texture assets.
	// TODO: this is also used by server.dll library.
	e1.Offset(0x213).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to unused VGUI code to prevent crash at SIGNONSTATE_PRESPAWN.
	e3.Offset(0xFB0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });

	//-------------------------------------------------------------------------
	// JNE --> JNP | Skip Video Mode initialization code.
	gCEngineAPI_OnStartup.Offset(0x5E).Patch({ 0xE9, 0xC6, 0x01, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JNP | Skip Video Mode validation code.
	gCEngineAPI_Init.Offset(0xB7).Patch({ 0xE9, 0xC7, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// JNE --> JNP | Skip CreateWindow Initialization code.
	//gCEngineAPI_ModInit.Offset(0x3DD).Patch({ 0xE9, 0xB5, 0x00, 0x00, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to texture and material preloading.
	gCEngineAPI_Connect.Offset(0xDD).Patch({ 0x90, 0x90, 0x90 });
	gCEngineAPI_Connect.Offset(0xF1).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
	gCEngineAPI_Connect.Offset(0x1C6).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90});
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to texture and material preloading.
	gCSourceAppSystemGroup_Create.Offset(0x35D).Patch({ 0x90, 0x90, 0x90 }); // joystickInit?
	//gCSourceAppSystemGroup_Create.Offset(0x384).Patch({ 0x90, 0x90, 0x90 }); // PrecacheMaterial
	gCSourceAppSystemGroup_Create.Offset(0x39E).Patch({ 0x90, 0x90, 0x90 }); // binkBlankTexture
	//-------------------------------------------------------------------------
	// FUN --> RET | Return early in 'CVideoMode_Common::DrawStartupGraphic'.
	gCVideoMode_Common__DrawStartupGraphic.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Return early in 'CShaderSystem::Init' to prevent initialization.
	//gCShaderSystem_Init.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// JMP --> RET | Return early to prevent 'InitDebugMaterials' from being executed.
	gInitMaterialSystem.Offset(0x7D).Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | Return early in 'RenderFrame?' (Called from VGUI and Host_Init).
	e8.Offset(0x44).Patch({ 0xE9, 0x41, 0x04, 0x00, 0x00 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to prevent texture creation.
	e9.Offset(0x6).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | NOP call to prevent texture creation.
	e9.Offset(0x4AB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// JMP --> RET | RET early to prevent 'PIXVIS' code execution.
	e9.Offset(0x4B5).Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | RET early to prevent '' code execution.
	e10.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// CAL --> NOP | Prevent memory allocation and population for shader assets.
	//gShaderDispatch.Offset(0x25).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	gShaderDispatch.Offset(0x3C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	gShaderDispatch.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	gShaderDispatch.Offset(0x56).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	gShaderDispatch.Offset(0x62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// FUN --> RET | RET early to prevent 'ShaderCreate' code execution.
	gShaderCreate.Patch({ 0xC3 });
	//-------------------------------------------------------------------------
	// FUN --> RET | RET early to prevent 'TextureCreate' code execution.
	gTextureCreate.Patch({ 0xC3 });

	OnLevelLoadingStarted.Patch({ 0xC3 });

	//-------------------------------------------------------------------------
	// START TESTING BLOCK
	//-------------------------------------------------------------------------

	MemoryAddress t0 = 0x00000001401D71E0;
	t0.Patch({ 0xC3 });
	MemoryAddress t1 = 0x0000000140456B50;
	t1.Offset(0x292).Patch({ 0xE9, 0xEE, 0x00, 0x00, 0x00 });
	MemoryAddress t2 = 0x0000000140238DA0;
	t2.Offset(0x4E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	MemoryAddress t3 = 0x0000000140312D80;
	//t3.Offset(0xB3).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	MemoryAddress t4 = 0x0000000140312D80; // Patch Additional shader preloading.
	//t4.Offset(0xB3).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	MemoryAddress t5 = 0x00000001403BBFD0;
	t5.Offset(0x7D8).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });


	// BSP --------------------------------------------------------------------
	MemoryAddress t6 = 0x00000001402546F0; // BSP.
	t6.Offset(0x200).Patch({ 0xEB, 0xA9 }); // Skip SKYLIGHTS.
	t6.Offset(0x352).Patch({ 0xEB, 0xA5 }); // Skip LUMP_LIGHTMAP.
	t6.Offset(0xBF2).Patch({ 0x90, 0x90 }); // Skip WORLDLIGHTS
	t6.Offset(0xDA9).Patch({ 0x90, 0x90 }); // Skip TWEAKLIGHTS
	t6.Offset(0xEEB).Patch({ 0xE9, 0x3D, 0x01, 0x00, 0x00 });
	//t6.Offset(0x61B).Patch({ 0xE9, 0xE2, 0x02, 0x00, 0x00 });

	// CSHADERSYSTEM ----------------------------------------------------------
	MemoryAddress t7 = 0x00000001403DFC30; // Unnecessary CShaderSystem call?
	t7.Offset(0x3).Patch({ 0xE9, 0x95, 0x03, 0x00, 0x00 });

	// UNKNOWN ----------------------------------------------------------------
	MemoryAddress t8 = 0x00000001403C0480;
	t8.Patch({ 0xC3 }); // Return from unknown call during ChangeLevel. (LATE)
	MemoryAddress t9 = 0x00000001403EE420;
	t9.Patch({ 0xC3 }); // Return from unknown call during ChangeLevel. (EARLY)

	//-------------------------------------------------------------------------
	// END TESTING BLOCK
	//-------------------------------------------------------------------------
}

// TEST
void SetCHostState()
{
	static std::string ServerMap = std::string();
	ServerMap = "mp_rr_canyonlands_64k_x_64k";
	strncpy_s(GameGlobals::HostState->m_levelName, ServerMap.c_str(), 64); // Copy new map into hoststate levelname. 64 is size of m_levelname.
	GameGlobals::HostState->m_iNextState = HostStates_t::HS_NEW_GAME; // Force CHostState::FrameUpdate to start a server.
}