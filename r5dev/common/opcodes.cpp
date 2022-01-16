/*-----------------------------------------------------------------------------
 * _opcodes.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "common/opcodes.h"
#include "engine/host_cmd.h"
#include "materialsystem/materialsystem.h"
#include "bsplib/bsplib.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "milessdk/win64_rrthreads.h"
#endif // !DEDICATED


#ifdef DEDICATED
void Dedicated_Init()
{
	*(uintptr_t*)0x14D415040 = 0x1417304E8; // CEngineClient::CEngineClient().
	//*(uintptr_t*)0x14B37C3C0 = 0x141F10CA0; // CHLClient::CHLClient().
	*(uintptr_t*)0x14B3800D7 = 0x1;         // bool bDedicated = true.

	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
	CVideoMode_Common__CreateGameWindow.Offset(0x2C).Patch({ 0xE9, 0x9A, 0x00, 0x00, 0x00 }); // PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized (STGS RPak datatype is registered here).

	//-------------------------------------------------------------------------
	// CHLClIENT
	//-------------------------------------------------------------------------
	gCHLClient__1000.Patch({ 0xC3 }); // FUN --> RET | Return early in unknown 'CHLClient' function to prevent infinite loop.

	//-------------------------------------------------------------------------
	// CSOURCEAPPSYSTEMGROUP
	//-------------------------------------------------------------------------
	gCSourceAppSystemGroup__Create.Offset(0x248).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x267).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | materials->Connect().
	//gCSourceAppSystemGroup__Create.Offset(0x286).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | mdlCache->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x2A5).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x2C4).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | avi->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x2E3).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Connect().
	//gCSourceAppSystemGroup__Create.Offset(0x302).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | dataCache->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x321).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matSystemSurface->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x340).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Connect().
	gCSourceAppSystemGroup__Create.Offset(0x35D).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Init().
	gCSourceAppSystemGroup__Create.Offset(0x384).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Init().
	gCSourceAppSystemGroup__Create.Offset(0x39E).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | bik->Init().
	gCSourceAppSystemGroup__Create.Offset(0x3AB).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Init().
	gCSourceAppSystemGroup__Create.Offset(0x3F6).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Init().
	gCSourceAppSystemGroup__Create.Offset(0x3E9).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matEmbeddedPanel->Init().
	gCSourceAppSystemGroup__Create.Offset(0x3F9).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | EAC_ClientInterface_Init().

	//-------------------------------------------------------------------------
	// CMATERIALSYSTEM
	//-------------------------------------------------------------------------
	//gCMaterialSystem__MatsysMode_Init.Offset(0x22).Patch({ 0xEB, 0x66 });        // JE  --> JMP | Matsys mode init (CMaterialSystem). // TODO: Needed?
	CMaterialSystem__Init.Offset(0x406).Patch({ 0xE9, 0x55, 0x05, 0x00, 0x00 }); // MOV --> JMP | Jump over material KeyValue definitions and 'CMatRenderContextBase::sm_RenderData([x])'.
	InitMaterialSystem.Offset(0x7D).Patch({ 0xC3 });                             // JMP --> RET | Return early to prevent 'InitDebugMaterials' from being executed. // RESEARCH NEEDED.

	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
	CShaderSystem__Init.Patch({ 0xC3 });                                         // FUN --> RET | Return early in 'CShaderSystem::Init' to prevent initialization.

	//-------------------------------------------------------------------------
	// CSTUDIORENDERCONTEXT
	//-------------------------------------------------------------------------
	// Note: The registers here seems to contains pointers to material data and 'CMaterial' class methods when the shader system is initialized.
	CStudioRenderContext__LoadModel.Offset(0x17D).Patch({ 0x90, 0x90, 0x90, 0x90 });             // MOV --> NOP | RAX + RCX are both nullptrs.
	CStudioRenderContext__LoadModel.Offset(0x181).Patch({ 0x90, 0x90, 0x90 });                   // MOV --> NOP | RCX is nullptr when trying to dereference.
	CStudioRenderContext__LoadModel.Offset(0x184).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
	CStudioRenderContext__LoadMaterials.Offset(0x28).Patch({ 0xE9, 0x80, 0x04, 0x00, 0x00 });    // FUN --> RET | 'CStudioRenderContext::LoadMaterials' is called virtually by the 'RMDL' streaming job.


	//-------------------------------------------------------------------------
	// CMODELLOADER
	//-------------------------------------------------------------------------
	CModelLoader__LoadModel.Offset(0x462).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });               // CAL --> NOP | Prevent call to 'CStudioRenderContext::LoadMaterials'.
	CModelLoader__Studio_LoadModel.Offset(0x325).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialSystem::FindMaterialEx' fails as RAX is nullptr.
	CModelLoader__Studio_LoadModel.Offset(0x33D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Studio_LoadModel.Offset(0x359).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Studio_LoadModel.Offset(0x374).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
	CModelLoader__Studio_LoadModel.Offset(0x38D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'ReturnZero' fails as RAX is nullptr.
	CModelLoader__Studio_LoadModel.Offset(0x3A4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.

	//-------------------------------------------------------------------------
	// CGAMESERVER
	//-------------------------------------------------------------------------
	CGameServer__SpawnServer.Offset(0x43).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent call to unknown material/shader code.
	CGameServer__SpawnServer.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // TODO: Research 'CIVDebugOverlay'.

	//-------------------------------------------------------------------------
	// MM_HEARTBEAT
	//-------------------------------------------------------------------------
	MM_Heartbeat__ToString.Offset(0xF).Patch({ 0xE9, 0x22, 0x01, 0x00, 0x00 });    // JS  --> JMP | Skip ListenServer HeartBeat.

	//-------------------------------------------------------------------------
	// RUNTIME: SYS_INITGAME
	//-------------------------------------------------------------------------
	Sys_InitGame.Offset(0x70).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // STZNZ --> NOP | Prevent 'bDedicated' from being set to false.

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_INIT
	//-------------------------------------------------------------------------
	gHost_Init_0.Offset(0xC2).Patch({ 0xEB, 0x34 });                    // CAL --> NOP | Disable 'vpk/client_common.bsp' loading.
	gHost_Init_0.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> JMP | Disable UI material asset initialization.
	gHost_Init_0.Offset(0x859).Patch({ 0xE9, 0x19, 0x04, 0x00, 0x00 }); // LEA --> RET | Disable 'client.dll' library initialization.
	gHost_Init_0.Offset(0xC77).Patch({ 0xE8, 0x44, 0xCF, 0xFF, 0xFF }); // CAL --> CAL | Disable user config loading and call entitlements.rson initialization instead.

	gHost_Init_1.Offset(0x609).Patch({ 0xEB, 0x2B });                   // JE  --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x621).Patch({ 0xEB, 0x0C });                   // JNE --> JMP | Skip client.dll Init_PostVideo() validation code.
	gHost_Init_1.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 }); // JE  --> JMP | Skip NULL call as client is never initialized.
	gHost_Init_1.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 }); // JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.

	gHost_Init_2.Offset(0x26F).Patch({ 0xE9, 0x4D, 0x05, 0x00, 0x00 }); // JNE --> JMP | client.dll systems initialization.

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_NEWGAME
	//-------------------------------------------------------------------------
	Host_NewGame.Offset(0x4E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
	Host_NewGame.Offset(0x637).Patch({ 0xE9, 0xC1, 0x00, 0x00, 0x00 }); // JNE --> JMP | Prevent connect localhost from being executed in Host_NewGame.

	//-------------------------------------------------------------------------
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
	_Host_RunFrame.Offset(0xFB0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });    // CAL --> NOP | NOP call to unused VGUI code to prevent crash at SIGNONSTATE_PRESPAWN.
	_Host_RunFrame.Offset(0x1023).Patch({ 0x90, 0x90, 0x90 });               // CAL --> NOP | NOP NULL call as client is never initialized.

	//-------------------------------------------------------------------------
	// RUNTIME: EBISUSDK
	//-------------------------------------------------------------------------
	p_EbisuSDK_SetState.Offset(0x0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN).Patch({ 0x0F, 0x85 }); // JE  --> JNZ | Prevent EbisuSDK from initializing on the engine and server.

	//-------------------------------------------------------------------------
	// RUNTIME: FAIRFIGHT
	//-------------------------------------------------------------------------
	FairFight_Init.Offset(0x0).FindPatternSelf("0F 87", ADDRESS::Direction::DOWN, 200).Patch({ 0x0F, 0x85 }); // JA  --> JNZ | Prevent 'FairFight' anti-cheat from initializing on the server by comparing RAX against 0x0 instead. Init will crash since the plugins aren't shipped.

	//-------------------------------------------------------------------------
	// RUNTIME: BSP_LUMP
	//-------------------------------------------------------------------------
	CollisionBSPData_LoadAllLumps.Offset(0x41).Patch({ 0xE9, 0x4F, 0x04, 0x00, 0x00 });   // JNE --> NOP | SKYLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0x974).Patch({ 0x90, 0x90 });                    // JE  --> NOP | VERTNORMALS.
	CollisionBSPData_LoadAllLumps.Offset(0xA55).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | MATERIALSORTS.
	CollisionBSPData_LoadAllLumps.Offset(0xA62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | MESHBOUNDS.
	CollisionBSPData_LoadAllLumps.Offset(0xA83).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | MESHVERTS.
	CollisionBSPData_LoadAllLumps.Offset(0xAC0).Patch({ 0x90, 0x90 });                    // JE  --> NOP | INDICES.
	CollisionBSPData_LoadAllLumps.Offset(0xBF2).Patch({ 0x90, 0x90 });                    // JE  --> NOP | WORLDLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0xDA9).Patch({ 0x90, 0x90 });                    // JE  --> NOP | TWEAKLIGHTS.
	CollisionBSPData_LoadAllLumps.Offset(0xEEB).Patch({ 0xE9, 0x3D, 0x01, 0x00, 0x00 });  // JLE --> JMP | Exception 0x57 in while trying to dereference [R15 + R14 *8 + 0x10].
	CollisionBSPData_LoadAllLumps.Offset(0x61B).Patch({ 0xE9, 0xE2, 0x02, 0x00, 0x00 });  // JZ  --> JMP | Prevent call to 'CMod_LoadTextures()'.
	CollisionBSPData_LoadAllLumps.Offset(0x1045).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent call to 'Mod_LoadCubemapSamples()'.

	CollisionBSPData_LinkPhysics.Offset(0x129).Patch({ 0x90, 0x90, 0x90 });               // MOV --> NOP | RCX is nullptr during dereference since shadersystem isn't initialized. Exception 'C0000005'.
	CollisionBSPData_LinkPhysics.Offset(0x12C).Patch({ 0x90, 0x90, 0x90 });               // CAL --> NOP | Virtual call to 'CTexture' class member in RAX + 0x78 fails. Previous instruction could not dereference.

	//-------------------------------------------------------------------------
	// RUNTIME: PROP_STATIC
	//-------------------------------------------------------------------------
	// Note: At [14028F3B0 + 0x5C7] RSP seems to contain a block of pointers to data for the static prop rmdl in question. [RSP + 0x70] is a pointer to (what seems to be) shader/material data. The pointer will be NULL without a shader system.
	p_CalcPropStaticFrustumCulling.Offset(0x5E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });        // MOV --> NOP | RSP + 0x70 is a nullptr which gets moved to R13, R13 gets used here resulting in exception 'C0000005'.
	p_CalcPropStaticFrustumCulling.Offset(0x5EB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.

	//-------------------------------------------------------------------------
	// RUNTIME: GL_SCREEN
	//-------------------------------------------------------------------------
	SCR_BeginLoadingPlaque.Offset(0x82).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // JNE --> JMP | virtual call to 'CHLClient::CHudMessage'.
	SCR_BeginLoadingPlaque.Offset(0xA4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // JNE --> JMP | virtual call to 'CEngineVGui::OnLevelLoadingStarted'.
	SCR_BeginLoadingPlaque.Offset(0x1D6).Patch({ 0xEB, 0x27 });                        // JNE --> JMP | Prevent connect command from crashing by invalid call to UI function.
}
#endif // DEDICATED

void RuntimePtc_Init() /* .TEXT */
{
#ifndef DEDICATED
	p_WASAPI_GetAudioDevice.Offset(0x410).FindPattern("FF 15 ?? ?? 01 00", ADDRESS::Direction::DOWN, 100).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xEB }); // CAL --> NOP | Disable debugger check when miles searches for audio device to allow attaching the debugger to the game upon launch.
	FairFight_Init.Offset(0x0).FindPatternSelf("0F 87", ADDRESS::Direction::DOWN, 200).Patch({ 0x0F, 0x85 });      // JA  --> JNZ | Prevent 'FairFight' anti-cheat from initializing on the server by comparing RAX against 0x0 instead. Init will crash since the plugins aren't shipped.
	SCR_BeginLoadingPlaque.Offset(0x1AD).FindPatternSelf("75 27", ADDRESS::Direction::DOWN).Patch({ 0xEB, 0x27 }); // JNE --> JMP | Prevent connect command from crashing by invalid call to UI function.
#endif // !DEDICATED
}

void RuntimePtc_Toggle() /* .TEXT */
{
#ifdef GAMEDLL_S3
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


/*
rtech_asyncload "common.rpak"
rtech_asyncload "common_mp.rpak"
rtech_asyncload "mp_rr_canyonlands_mu1.rpak"
rtech_asyncload "mp_rr_desertlands_64k_x_64k.rpak"
*/
#endif // GAMEDLL_S3
}
