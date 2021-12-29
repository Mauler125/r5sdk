#include "core/stdafx.h"
#include "tier0/basetypes.h"
#include "common/opcodes.h"
#include "engine/host_cmd.h"
#include "bsplib/bsplib.h"

 /*-----------------------------------------------------------------------------
  * _opcodes.cpp
  *-----------------------------------------------------------------------------*/

#ifdef DEDICATED

void NoShaderApi()
{
	//-------------------------------------------------------------------------
	// -NOSHADERAPI
	//-------------------------------------------------------------------------
	gCShaderSystem__Init.Patch({ 0xC3 });                                                         // FUN --> RET | Return early in 'CShaderSystem::Init' to prevent initialization.

	gCGameServer__SpawnServer.Offset(0x43).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });               // CAL --> NOP | Prevent call to unknown materia;/shader code.
	gCGameServer__SpawnServer.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });               // TODO: Research 'CIVDebugOverlay'.

	CStudioRenderContext__LoadMaterials.Offset(0x28).Patch({ 0xE9, 0x80, 0x04, 0x00, 0x00 });     // FUN --> RET | 'CStudioRenderContext::LoadMaterials' is called virtually by the 'RMDL' streaming job.

	// Note: The registers here seems to contains pointers to material data and 'CMaterial' class methods when the shader system is initialized.
	gCStudioRenderContext__LoadModel.Offset(0x17D).Patch({ 0x90, 0x90, 0x90, 0x90 });             // MOV --> NOP | RAX + RCX are both nullptrs.
	gCStudioRenderContext__LoadModel.Offset(0x181).Patch({ 0x90, 0x90, 0x90 });                   // MOV --> NOP | RCX is nullptr when trying to dereference.
	gCStudioRenderContext__LoadModel.Offset(0x184).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.

	CollisionBSPData_LoadAllLumps.Offset(0x1045).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });         // CAL --> NOP | Prevent call to 'Mod_LoadCubemapSamples()'

	LoadModel.Offset(0x462).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });                              // CAL --> NOP | Prevent call to 'CStudioRenderContext::LoadMaterials'.
	//LoadModel.Offset(0x6FE).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });                              // CAL --> NOP | Unknown material/texture code.

	CModelLoader__Sprite_LoadModel.Offset(0x325).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'CMaterialSystem::FindMaterialEx' fails as RAX is nullptr.
	CModelLoader__Sprite_LoadModel.Offset(0x33D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Sprite_LoadModel.Offset(0x359).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Sprite_LoadModel.Offset(0x374).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Sprite_LoadModel.Offset(0x38D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'ReturnZero' fails as RAX is nullptr.
	CModelLoader__Sprite_LoadModel.Offset(0x3A4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.

	// Note: At [14028F3B0 + 0x5C7] RSP seems to contain a block of pointers to data for the static prop rmdl in question. [RSP + 0x70] is a pointer to (what seems to be) shader/material data. The pointer will be NULL without a shader system.
	p_CalcPropStaticFrustumCulling.Offset(0x5E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });         // MOV --> NOP | RSP + 0x70 is a nullptr which gets moved to R13, R13 gets used here resulting in exception 'C0000005'.
	p_CalcPropStaticFrustumCulling.Offset(0x5EB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });   // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
}

void Dedicated_Init()
{
	*(uintptr_t*)0x14D415040 = 0x1417304E8;
	*(uintptr_t*)0x14B37C3C0 = 0x141F10CA0;
	*(uintptr_t*)0x14B3800D7 = 0x1; // bDedicated

	NoShaderApi();

	//-------------------------------------------------------------------------
	// RESEARCH FOR IMPROVEMENT!
	//-------------------------------------------------------------------------
	e10.Patch({ 0xC3 }); // FUN --> RET | RET early to prevent '' code execution.
	e8.Offset(0x44).Patch({ 0xE9, 0x41, 0x04, 0x00, 0x00 });                        // FUN --> RET | Return early in 'RenderFrame?' (Called from VGUI and Host_Init).
	gInitMaterialSystem.Offset(0x7D).Patch({ 0xC3 });                               // JMP --> RET | Return early to prevent 'InitDebugMaterials' from being executed.
	e3.Offset(0xFB0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });                       // CAL --> NOP | NOP call to unused VGUI code to prevent crash at SIGNONSTATE_PRESPAWN.
	addr_CEngine_Frame.Offset(0x410).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | CHLClient call inside eng->frame.
	FairFight_Init.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });      // JA  --> JMP | Prevent FairFight anti-cheat from initializing on the server.
	s1.Offset(0x1023).Patch({ 0x90, 0x90, 0x90 });                                  // CAL --> NOP | NOP NULL call as client is never initialized.
	s2.Offset(0xF).Patch({ 0xE9, 0x22, 0x01, 0x00, 0x00 });                         // JS  --> JMP | Skip ListenServer HeartBeat.
	e1.Offset(0x213).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });                 // JNE --> NOP | Skip settings field loading for client texture assets.
	e9.Offset(0x6).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });                         // CAL --> NOP | NOP call to prevent texture creation.
	gShaderCreate.Patch({ 0xC3 });                                                  // FUN --> RET | RET early to prevent 'ShaderCreate' code execution.
	gTextureCreate.Patch({ 0xC3 });                                                 // FUN --> RET | RET early to prevent 'TextureCreate' code execution.
	c2.Offset(0x23C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });                 // JNE --> NOP | TODO: NOP 'particle_script' instead.
	c2.Offset(0x2BD).Patch({ 0x90, 0x90, 0x90 });                                   // MOV --> NOP | TODO: NOP 'particle_script' instead.
	c3.Offset(0xA9).Patch({ 0x90, 0x90, 0x90, 0x90 });                              // MOV --> NOP | TODO: NOP 'highlight_system' instead.
	unk1.Offset(0x129).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });               // MOV AND CALL --> | Eliminates null pointer dereference crash. Needs more research.

	//-------------------------------------------------------------------------
	// CENGINEAPI
	//-------------------------------------------------------------------------
	gCEngineAPI__Init.Offset(0xB7).Patch({ 0xE9, 0xC7, 0x00, 0x00, 0x00 });           // JNE --> JNP | Skip Video Mode validation code.
	gCEngineAPI__OnStartup.Offset(0x5E).Patch({ 0xE9, 0xC6, 0x01, 0x00, 0x00 });      // JNE --> JNP | Skip Video Mode initialization code.
	gCEngineAPI__Connect.Offset(0xDD).Patch({ 0x90, 0x90, 0x90 });                    // CAL --> NOP | NOP call to texture and material preloading.
	gCEngineAPI__Connect.Offset(0xF1).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | NOP call to texture and material preloading.
	gCEngineAPI__Connect.Offset(0x1C6).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | NOP call to texture and material preloading.
	//gCEngineAPI__ModInit.Offset(0x3DD).Patch({ 0xE9, 0xB5, 0x00, 0x00, 0x00, 0x00 }); // JNE --> JNP | Skip CreateWindow Initialization code.
	gCEngineAPI__ModInit.Offset(0x44C).Patch({ 0xEB, 0x49 });                         // JNZ --> JMP | Skip CreateGameWindow validation code.
	//gCEngineAPI__ModInit.Offset(0x3DD).Patch({ 0xEB, 0x6D });                         // JE  --> JMP | Skip CreateGameWindow initialization code.

	//-------------------------------------------------------------------------
	// CENGINEVGUI
	//-------------------------------------------------------------------------
	gCEngineVGui__Init.Patch({ 0x48, 0x33, 0xC0, 0xC3, 0x90, 0x90, 0x90 }); // CMP --> XOR | Skip VGUI initialization jumptable.
	gCEngineVGui__OnLevelLoadingStarted.Patch({ 0xC3 });                    // FUN --> RET | 

	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
	gCGame__CreateGameWindow.Offset(0x2C).Patch({ 0xE9, 0x9A, 0x00, 0x00, 0x00 }); // PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized.

	//-------------------------------------------------------------------------
	// CHLClIENT
	//-------------------------------------------------------------------------
	gCHLClient__1000.Patch({ 0xC3 });       // FUN --> RET | Return early in 'gCHLClient::unnamed' to prevent infinite loop.
	gCHLClient__HudMessage.Patch({ 0xC3 }); // FUN --> RET | Return early from 'CHudMessage' call.

	//-------------------------------------------------------------------------
	// CSOURCEAPPSYSTEMGROUP
	//-------------------------------------------------------------------------
	gCSourceAppSystemGroup__Create.Offset(0x2A5).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}); // CAL --> NOP | studioRender->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x35D).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | joystickInit?
	gCSourceAppSystemGroup__Create.Offset(0x384).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | PrecacheMaterial.
	gCSourceAppSystemGroup__Create.Offset(0x39E).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | binkBlankTexture.

	//-------------------------------------------------------------------------
	// CVIDEOMODE_COMMON
	//-------------------------------------------------------------------------
	gCVideoMode_Common__DrawStartupGraphic.Patch({ 0xC3 }); // FUN --> RET | Return early in 'CVideoMode_Common::DrawStartupGraphic'.

	//-------------------------------------------------------------------------
	// CMATERIALSYSTEM
	//-------------------------------------------------------------------------
	gCMaterialSystem__MatsysMode_Init.Offset(0x22).Patch({ 0xEB, 0x66 }); // JE  --> JMP | Matsys mode init (CMaterialSystem).

	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
	gCShaderSystem__9.Offset(0x3).Patch({ 0xE9, 0x95, 0x03, 0x00, 0x00 }); // Unnecessary CShaderSystem call?

	//-------------------------------------------------------------------------
	// CSHADERGLUE
	//-------------------------------------------------------------------------
	gCShaderGlue__Init.Patch({ 0xC3 }); // FUN --> RET | Skip ShaderSetup(). CShaderGlue.

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_INIT
	//-------------------------------------------------------------------------
	gHost_Init_0.Offset(0xC2).Patch({ 0xEB, 0x34 });                    // CAL --> NOP | Disable 'vpk/client_common.bsp' loading.
	gHost_Init_0.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> JMP | Disable UI material asset initialization.
	gHost_Init_0.Offset(0x859).Patch({ 0xE9, 0x19, 0x04, 0x00, 0x00 }); // LEA --> RET | Disable 'client.dll' library initialization.
	gHost_Init_0.Offset(0xC77).Patch({ 0xE8, 0x44, 0xCF, 0xFF, 0xFF }); // CAL --> CAL | Disable user config loading and call entitlements.rson initialization instead.
	gHost_Init_1.Offset(0x19).Patch({ 0xEB, 0x6E });                    // JNE --> JMP | Take dedicated initialization routine instead.
	gHost_Init_1.Offset(0x609).Patch({ 0xEB, 0x2B });                   // JE  --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x621).Patch({ 0xEB, 0x0C });                   // JNE --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 }); // JE  --> JMP | Skip NULL call as client is never initialized.
	gHost_Init_1.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 }); // JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.
	//gHost_Init_2.Offset(0x5D8).Patch({ 0xEB, 0x05 });                   // JE  --> JMP | Render?

	//-------------------------------------------------------------------------
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
	//s1.Offset(0x1C6).Patch({ 0xE9, 0xAD, 0x11, 0x00, 0x00 }); // JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.
	//s1.Offset(0x1010).Patch({ 0xEB, 0x14 });                  // JNE --> JMP | Return early in _Host_RunFrame() for debugging perposes.

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_NEWGAME
	//-------------------------------------------------------------------------
	Host_NewGame.Offset(0x637).Patch({ 0xE9, 0xC1, 0x00, 0x00, 0x00 }); // JNE --> JMP | Prevent connect localhost from being executed in Host_NewGame.

	//-------------------------------------------------------------------------
	// RUNTIME: EBISUSDK
	//-------------------------------------------------------------------------
	Origin_Init.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 }); // JNZ --> JMP | Prevent EbisuSDK from initializing on the engine and server.
	Origin_SetState.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00 });   // JNZ --> JMP | Prevent EbisuSDK from initializing on the engine and server.

	//-------------------------------------------------------------------------
	// RUNTIME: FAIRFIGHT
	//-------------------------------------------------------------------------
	FairFight_Init.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });

	//-------------------------------------------------------------------------
	// RUNTIME: BSP_LUMP
	//-------------------------------------------------------------------------
	CollisionBSPData_LoadAllLumps.Offset(0x41).Patch({ 0xE9, 0x4F, 0x04, 0x00, 0x00 });  // JNE --> NOP | SKYLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0x974).Patch({ 0x90, 0x90 });                   // JE  --> NOP | VERTNORMALS.
	CollisionBSPData_LoadAllLumps.Offset(0xA55).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | MATERIALSORTS.
	CollisionBSPData_LoadAllLumps.Offset(0xA62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | MESHBOUNDS.
	CollisionBSPData_LoadAllLumps.Offset(0xA83).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | MESHVERTS.
	CollisionBSPData_LoadAllLumps.Offset(0xAC0).Patch({ 0x90, 0x90 });                   // JE  --> NOP | INDICES.
	CollisionBSPData_LoadAllLumps.Offset(0xBF2).Patch({ 0x90, 0x90 });                   // JE  --> NOP | WORLDLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0xDA9).Patch({ 0x90, 0x90 });                   // JE  --> NOP | TWEAKLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0xEEB).Patch({ 0xE9, 0x3D, 0x01, 0x00, 0x00 });
	CollisionBSPData_LoadAllLumps.Offset(0x61B).Patch({ 0xE9, 0xE2, 0x02, 0x00, 0x00 });

	//-------------------------------------------------------------------------
	// RUNTIME: RENDERING
	//-------------------------------------------------------------------------
	r0.Patch({ 0xC3 });                                       // FUN --> RET | Called from CEngineClient and CEngineVGUI (Init()?).
	gMatSync.Patch({ 0xC3 });                                 // FUN --> RET | Skip Matsync. Called from CMaterialSystem. TODO: Return in root caller.
	r4.Patch({ 0xC3 });                                       // FUN --> RET | Clear render buffer? Called from CMatRenderContext and CTexture.
	r5.Patch({ 0xC3 });                                       // FUN --> RET | Heavy render stuff. Called from CMatRenderContext.
	r6.Patch({ 0xC3 });                                       // FUN --> RET | Set shader resource.
	r7.Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90 });               // FUN --> RET | Return early in lightmap and post processing code.
	r8.Patch({ 0xC3, 0x90, 0x90, 0x90, 0x90, 0x90 });         // FUN --> RET | Return early.
	e9.Offset(0x4A6).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | NOP call to prevent shader dispatch.
	e9.Offset(0x4AB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | NOP call to prevent texture creation.
	e9.Offset(0x4B5).Patch({ 0xC3 });                         // JMP --> RET | RET early to prevent 'PIXVIS' code execution.

	//-------------------------------------------------------------------------
	// RUNTIME: USERINTERFACE
	//-------------------------------------------------------------------------
	SCR_BeginLoadingPlaque.Offset(0x427).Patch({ 0xEB, 0x09 }); // JNE --> JMP | Skip call to VGUI 'SCR_BeginLoadingPlaque'.

	//-------------------------------------------------------------------------
	// RUNTIME: RPAK_DISPATCH
	//-------------------------------------------------------------------------
	//gShaderDispatch.Offset(0x25).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	gShaderDispatch.Offset(0x3C).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent memory allocation and population for shader assets.
	gShaderDispatch.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent memory allocation and population for shader assets.
	gShaderDispatch.Offset(0x56).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent memory allocation and population for shader assets.
	gShaderDispatch.Offset(0x62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent memory allocation and population for shader assets.

	// UNKNOWN ----------------------------------------------------------------
	ADDRESS t8 = 0x00000001403C0480;
	t8.Patch({ 0xC3 }); // Return from unknown call during ChangeLevel. [LATE]
	ADDRESS t9 = 0x00000001403EE420;
	t9.Patch({ 0xC3 }); // Return from unknown call during ChangeLevel. [EARLY]
	//-------------------------------------------------------------------------
	// RUNTIME BLOCK
	//-------------------------------------------------------------------------
	ADDRESS t0 = 0x00000001401D71E0;
	//t0.Patch({ 0xC3 });                                               // RPak unload?
	ADDRESS t1 = 0x0000000140456B50;
	t1.Offset(0x292).Patch({ 0xE9, 0xEE, 0x00, 0x00, 0x00 });
	ADDRESS t2 = 0x0000000140238DA0;
	t2.Offset(0x4E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	ADDRESS t3 = 0x0000000140312D80;
	//t3.Offset(0xB3).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	ADDRESS t4 = 0x0000000140312D80; // Patch Additional shader preloading.
	//t4.Offset(0xB3).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	ADDRESS t5 = 0x00000001403BBFD0;
	t5.Offset(0x7D8).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	//-------------------------------------------------------------------------
	// END RUNTIME BLOCK
	//-------------------------------------------------------------------------
}
#endif // DEDICATED

void RuntimePtc_Init() /* .TEXT */
{
#ifdef DEDICATED
	//-------------------------------------------------------------------------
	// JNZ --> JMP | Prevent OriginSDK from initializing on the server
	Origin_Init.Offset(0x0B).Patch({ 0xE9, 0x63, 0x02, 0x00, 0x00, 0x00 });
	Origin_SetState.Offset(0x0E).Patch({ 0xE9, 0xCB, 0x03, 0x00, 0x00, 0x00 });
#endif // DEDICATED
	//-------------------------------------------------------------------------
	// JNE --> JMP | Allow games to be loaded without the optional texture streaming file
	//WriteProcessMemory(GameProcess, LPVOID(dst002 + 0x8E5), "\xEB\x19", 2, NULL);
	//-------------------------------------------------------------------------
	// JNE --> JMP | Prevent connect command from crashing by invalid call to UI function
	dst003.Offset(0x1D6).Patch({ 0xEB, 0x27 });
	//-------------------------------------------------------------------------
	// JA  --> JMP | Prevent FairFight anti-cheat from initializing on the 
	FairFight_Init.Offset(0x61).Patch({ 0xE9, 0xED, 0x00, 0x00, 0x00, 0x00 });
}

void RuntimePtc_Toggle() /* .TEXT */
{
	static bool g_nop = true;

	if (g_nop)
	{
		//-------------------------------------------------------------------------
		// CALL --> NOP | Allow some maps to be loaded by nopping out a call in LoadProp function
		dst007.Offset(0x5E8).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });


		//-------------------------------------------------------------------------
		// CALL --> NOP | Disable the viewmodel rendered to avoid a crash from a certain entity in desertlands_mu1
		//dst008.Offset(0x67).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });


		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>>| TEXT OPCODES OVERWRITTEN |<<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		//-------------------------------------------------------------------------
		// NOP --> CALL | Recover function DST007
		//WriteProcessMemory(GameProcess, LPVOID(dst007 + 0x5E8), "\x48\x8B\x03\xFF\x90\xB0\x02\x00\x00\x84\xC0", 11, NULL);

		dst007.Offset(0x5E8).Patch({ 0x48, 0x8B, 0x03, 0xFF, 0x90, 0xB0, 0x02, 0x00, 0x00, 0x84, 0xC0 });
		//-------------------------------------------------------------------------
		// NOP --> CALL | Recover function DST008
		//dst008.Offset(0x67).Patch({ 0xE8, 0x54, 0xD8, 0xFF, 0xFF });

		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>>>| TEXT OPCODES RECOVERED |<<<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	g_nop = !g_nop;
}
