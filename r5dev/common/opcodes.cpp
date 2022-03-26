/*-----------------------------------------------------------------------------
 * _opcodes.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "launcher/IApplication.h"
#include "common/opcodes.h"
#include "common/netmessages.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_cmd.h"
#include "engine/gl_screen.h"
#include "engine/gl_matsysiface.h"
#include "engine/modelloader.h"
#include "engine/cl_main.h"
#include "engine/sv_main.h"
#include "engine/sys_getmodes.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/fairfight_impl.h"
#include "rtech/rtech_game.h"
#include "client/cdll_engine_int.h"
#include "materialsystem/cmaterialsystem.h"
#include "studiorender/studiorendercontext.h"
#include "squirrel/sqvm.h"
#include "bsplib/bsplib.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "milessdk/win64_rrthreads.h"
#endif // !DEDICATED

#ifdef DEDICATED
 //-------------------------------------------------------------------------
 // Purpose: change runtime behavior
 //-------------------------------------------------------------------------
void Dedicated_Init()
{
	*(uintptr_t*)0x14D415040 = 0x1417304E8; // g_pEngineClient.
	*(uintptr_t*)0x14B3800D7 = 0x1;         // bool bDedicated = true.

	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
	{
		p_CVideoMode_Common__CreateGameWindow.Offset(0x2C).Patch({ 0xE9, 0x9A, 0x00, 0x00, 0x00 }); // PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized (STGS RPak datatype is registered here).
	}

	//-------------------------------------------------------------------------
	// CHLClIENT
	//-------------------------------------------------------------------------
	{
		p_CHLClient_LevelShutdown.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 }); // FUN --> RET | Return early in 'CHLClient::LevelShutdown()' during DLL shutdown.
		p_CHLClient_HudProcessInput.Patch({ 0xC3 });                             // FUN --> RET | Return early in 'CHLClient::HudProcessInput()' to prevent infinite loop.
	}

	//-------------------------------------------------------------------------
	// CSOURCEAPPSYSTEMGROUP
	//-------------------------------------------------------------------------
	{
		p_CSourceAppSystemGroup__Create.Offset(0x248).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x267).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | materials->Connect().
		//p_CSourceAppSystemGroup__Create.Offset(0x286).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | mdlCache->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x2A5).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x2C4).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | avi->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x2E3).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Connect().
		//p_CSourceAppSystemGroup__Create.Offset(0x302).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | dataCache->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x321).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matSystemSurface->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x340).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Connect().
		p_CSourceAppSystemGroup__Create.Offset(0x35D).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x384).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x39E).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | bik->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x3AB).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x3F6).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x3E9).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matEmbeddedPanel->Init().
		p_CSourceAppSystemGroup__Create.Offset(0x3F9).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | EAC_ClientInterface_Init().
	}

	//-------------------------------------------------------------------------
	// CMATERIALSYSTEM
	//-------------------------------------------------------------------------
	{
		//gCMaterialSystem__MatsysMode_Init.Offset(0x22).Patch({ 0xEB, 0x66 });        // JE  --> JMP | Matsys mode init (CMaterialSystem). // TODO: Needed?
		CMaterialSystem__Init.Offset(0x406).Patch({ 0xE9, 0x55, 0x05, 0x00, 0x00 }); // MOV --> JMP | Jump over material KeyValue definitions and 'CMatRenderContextBase::sm_RenderData([x])'.
		InitMaterialSystem.Offset(0x7D).Patch({ 0xC3 });                             // JMP --> RET | Return early to prevent 'InitDebugMaterials' from being executed. // RESEARCH NEEDED.
	}

	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
	{
		CShaderSystem__Init.Patch({ 0xC3 });                                         // FUN --> RET | Return early in 'CShaderSystem::Init()' to prevent initialization.
	}

	//-------------------------------------------------------------------------
	// CSTUDIORENDERCONTEXT
	//-------------------------------------------------------------------------
	{
		// Note: The registers here seems to contains pointers to material data and 'CMaterial' class methods when the shader system is initialized.
		CStudioRenderContext__LoadModel.Offset(0x17D).Patch({ 0x90, 0x90, 0x90, 0x90 });             // MOV --> NOP | RAX + RCX are both nullptrs.
		CStudioRenderContext__LoadModel.Offset(0x181).Patch({ 0x90, 0x90, 0x90 });                   // MOV --> NOP | RCX is nullptr when trying to dereference.
		CStudioRenderContext__LoadModel.Offset(0x184).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
		CStudioRenderContext__LoadMaterials.Offset(0x28).Patch({ 0xE9, 0x80, 0x04, 0x00, 0x00 });    // FUN --> RET | 'CStudioRenderContext::LoadMaterials' is called virtually by the 'RMDL' streaming job.
	}

	//-------------------------------------------------------------------------
	// CMODELLOADER
	//-------------------------------------------------------------------------
	{
		p_CModelLoader__LoadModel.Offset(0x462).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });               // CAL --> NOP | Prevent call to 'CStudioRenderContext::LoadMaterials'.
		p_CModelLoader__UnloadModel.Offset(0x129).Patch({ 0x90, 0x90, 0x90 });                         // MOV --> NOP | Virtual call to 'CShaderSystem' class method fails as RCX is nullptr.
		p_CModelLoader__UnloadModel.Offset(0x12C).Patch({ 0x90, 0x90, 0x90 });                         // CAL --> NOP | Virtual call to 'CTexture' class member in RAX + 0x78 fails. Previous instruction could not dereference.
		p_CModelLoader__Studio_LoadModel.Offset(0x325).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialSystem::FindMaterialEx' fails as RAX is nullptr.
		p_CModelLoader__Studio_LoadModel.Offset(0x33D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
		p_CModelLoader__Studio_LoadModel.Offset(0x359).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
		p_CModelLoader__Studio_LoadModel.Offset(0x374).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
		p_CModelLoader__Studio_LoadModel.Offset(0x38D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'ReturnZero' fails as RAX is nullptr.
		p_CModelLoader__Studio_LoadModel.Offset(0x3A4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.

		p_CModelLoader__Map_LoadModelGuts.Offset(0x41).Patch({ 0xE9, 0x4F, 0x04, 0x00, 0x00 });        // JNE --> NOP | SKYLIGHTS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0x974).Patch({ 0x90, 0x90 });                         // JE  --> NOP | VERTNORMALS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xA55).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MATERIALSORTS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xA62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MESHBOUNDS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xA83).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MESHVERTS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xAC0).Patch({ 0x90, 0x90 });                         // JE  --> NOP | INDICES.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xBF2).Patch({ 0x90, 0x90 });                         // JE  --> NOP | WORLDLIGHTS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xDA9).Patch({ 0x90, 0x90 });                         // JE  --> NOP | TWEAKLIGHTS.
		p_CModelLoader__Map_LoadModelGuts.Offset(0xEEB).Patch({ 0xE9, 0x3D, 0x01, 0x00, 0x00 });       // JLE --> JMP | Exception 0x57 in while trying to dereference [R15 + R14 *8 + 0x10].
		p_CModelLoader__Map_LoadModelGuts.Offset(0x61B).Patch({ 0xE9, 0xE2, 0x02, 0x00, 0x00 });       // JZ  --> JMP | Prevent call to 'CMod_LoadTextures()'.
		p_CModelLoader__Map_LoadModelGuts.Offset(0x1045).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });      // CAL --> NOP | Prevent call to 'Mod_LoadCubemapSamples()'.
	}

	//-------------------------------------------------------------------------
	// CGAMESERVER
	//-------------------------------------------------------------------------
	{
		CGameServer__SpawnServer.Offset(0x43).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent call to unknown material/shader code.
		CGameServer__SpawnServer.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | TODO: Research 'CIVDebugOverlay'.
	}

	//-------------------------------------------------------------------------
	// CVGUI
	//-------------------------------------------------------------------------
	{
		/*MOV EAX, 0*/
		CVGui__RunFrame.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                 // FUN --> RET | 'CVGui::RunFrame()' gets called on DLL shutdown.
	}

	//-------------------------------------------------------------------------
	// CENGINEVGUI
	//-------------------------------------------------------------------------
	{
		CEngineVGui__Shutdown.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                                  // FUN --> RET | Cannot shutdown CEngineVGui if its never initialized.
		CEngineVGui__ActivateGameUI.FindPatternSelf("74 08", ADDRESS::Direction::DOWN).Patch({ 0x90, 0x90 }); // JZ  --> NOP | Remove condition to return early when engine attempts to activate UI on the server.
	}

	//-------------------------------------------------------------------------
	// MM_HEARTBEAT
	//-------------------------------------------------------------------------
	{
		MM_Heartbeat__ToString.Offset(0xF).Patch({ 0xE9, 0x22, 0x01, 0x00, 0x00 });    // JS  --> JMP | Skip ListenServer HeartBeat.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: SYS_INITGAME
	//-------------------------------------------------------------------------
	{
		Sys_InitGame.Offset(0x70).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // STZNZ --> NOP | Prevent 'bDedicated' from being set to false.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_INIT
	//-------------------------------------------------------------------------
	{
		gHost_Init_0.Offset(0xC2).Patch({ 0xEB, 0x34 });                    // CAL --> NOP | Disable 'vpk/client_common.bsp' loading.
		gHost_Init_0.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> JMP | Disable UI material asset initialization.
		gHost_Init_0.Offset(0x859).Patch({ 0xE9, 0x19, 0x04, 0x00, 0x00 }); // LEA --> RET | Disable 'client.dll' library initialization.
		gHost_Init_0.Offset(0xC77).Patch({ 0xE8, 0x44, 0xCF, 0xFF, 0xFF }); // CAL --> CAL | Disable user config loading and call entitlements.rson initialization instead.

		gHost_Init_1.Offset(0x609).Patch({ 0xEB, 0x2B });                   // JE  --> JMP | Skip client.dll 'Init_PostVideo()' validation code.
		gHost_Init_1.Offset(0x621).Patch({ 0xEB, 0x0C });                   // JNE --> JMP | Skip client.dll 'Init_PostVideo()' validation code.
		gHost_Init_1.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 }); // JE  --> JMP | Skip NULL call as client is never initialized.
		gHost_Init_1.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 }); // JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.

		gHost_Init_2.Offset(0x26F).Patch({ 0xE9, 0x4D, 0x05, 0x00, 0x00 }); // JNE --> JMP | client.dll systems initialization.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_SHUTDOWN
	//-------------------------------------------------------------------------
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		Host_Shutdown.Offset(0x3B0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN).Patch({ 0x0F, 0x85 });      // JE  --> JNE | Cannot shutdown ClientDLL if its never initialized.
		Host_Shutdown.Offset(0x9D0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN, 300).Patch({ 0x0F, 0x85 }); // JE  --> JNE | Cannot shutdown EngineVGui if its never initialized.
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		Host_Shutdown.Offset(0x2B0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN, 300).Patch({ 0x0F, 0x85 }); // JE  --> JNE | Cannot shutdown ClientDLL if its never initialized.
		Host_Shutdown.Offset(0x5C0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN, 300).Patch({ 0x0F, 0x85 }); // JE  --> JNE | Cannot shutdown EngineVGui if its never initialized.
#endif
	}

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_NEWGAME
	//-------------------------------------------------------------------------
	{
		p_Host_NewGame.Offset(0x4E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });
		p_Host_NewGame.Offset(0x637).Patch({ 0xE9, 0xC1, 0x00, 0x00, 0x00 });      // JNE --> JMP | Prevent connect localhost from being executed in Host_NewGame.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
	{
		_Host_RunFrame.Offset(0xFB0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });    // CAL --> NOP | NOP call to unused VGUI code to prevent crash at SIGNONSTATE_PRESPAWN.
		_Host_RunFrame.Offset(0x1023).Patch({ 0x90, 0x90, 0x90 });               // CAL --> NOP | NOP NULL call as client is never initialized.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_DISCONNECT
	//-------------------------------------------------------------------------
	{
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		Host_Disconnect.Offset(0x4A).FindPatternSelf("FF 90 80", ADDRESS::Direction::DOWN, 300).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, }); // CAL --> RET | This seems to call 'CEngineVGui::GetGameUIInputContext()'.
#endif
	}

	//-------------------------------------------------------------------------
	// RUNTIME: RTECH_GAME
	//-------------------------------------------------------------------------
	{
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_RTech_LoadPak.Offset(0x890).FindPatternSelf("75", ADDRESS::Direction::DOWN, 200).Patch({ 0xEB });       // JNZ --> JMP | Disable error handling for missing streaming files on the server. The server does not need streamed data from the starpak files.
#endif
	}

	//-------------------------------------------------------------------------
	// RUNTIME: EBISUSDK
	//-------------------------------------------------------------------------
	{
		p_EbisuSDK_SetState.Offset(0x0).FindPatternSelf("0F 84", ADDRESS::Direction::DOWN).Patch({ 0x0F, 0x85 }); // JE  --> JNZ | Prevent EbisuSDK from initializing on the engine and server.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: FAIRFIGHT
	//-------------------------------------------------------------------------
	{
		FairFight_Init.Offset(0x0).FindPatternSelf("0F 87", ADDRESS::Direction::DOWN, 200).Patch({ 0x0F, 0x85 }); // JA  --> JNZ | Prevent 'FairFight' anti-cheat from initializing on the server by comparing RAX against 0x0 instead. Init will crash since the plugins aren't shipped.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: PROP_STATIC
	//-------------------------------------------------------------------------
	{
		// Note: At [14028F3B0 + 0x5C7] RSP seems to contain a block of pointers to data for the static prop rmdl in question. [RSP + 0x70] is a pointer to (what seems to be) shader/material data. The pointer will be NULL without a shader system.
		p_CalcPropStaticFrustumCulling.Offset(0x5E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });        // MOV --> NOP | RSP + 0x70 is a nullptr which gets moved to R13, R13 gets used here resulting in exception 'C0000005'.
		p_CalcPropStaticFrustumCulling.Offset(0x5EB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: GL_SCREEN
	//-------------------------------------------------------------------------
	{
		SCR_BeginLoadingPlaque.Offset(0x5B).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | virtual call to 'CHLClient::MilesQueueEvent'.
		SCR_BeginLoadingPlaque.Offset(0x82).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | virtual call to 'CHLClient::CHudMessage'.
		SCR_BeginLoadingPlaque.Offset(0xA4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | virtual call to 'CEngineVGui::OnLevelLoadingStarted'.
		SCR_BeginLoadingPlaque.Offset(0x1D6).Patch({ 0xEB, 0x27 });                        // JNE --> JMP | Prevent connect command from crashing by invalid call to UI function.
	}

	//-------------------------------------------------------------------------
	// RUNTIME: CL_CLEARSTATE
	//-------------------------------------------------------------------------
#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	{
		CL_ClearState.Offset(0x0).Patch({ 0xC3 });                                         // FUN --> RET | Invalid 'CL_ClearState()' call from Host_Shutdown causing segfault.
	}
#endif

	// This mandatory pak file should only exist on the client.
	if (!FileExists("vpk\\client_frontend.bsp.pak000_000.vpk"))
	{
		// Patch 'client' pak file string constants to 'server' if this is a standalone dedicated server.
		g_pClientVPKDir.PatchString("vpk/%sserver_%s.bsp.pak000%s");
		g_pClientBSP.PatchString("vpk/server_%s.bsp");
		g_pClientCommonBSP.PatchString("vpk/server_mp_common.bsp");
		g_pClientMPLobby.PatchString("vpk/server_mp_lobby");
		g_pClientMP.PatchString("vpk/server_mp_");
		g_pClientSP.PatchString("vpk/server_sp_");
	}
}
#endif // DEDICATED

void RuntimePtc_Init() /* .TEXT */
{
#ifndef DEDICATED
	p_WASAPI_GetAudioDevice.Offset(0x410).FindPattern("FF 15 ?? ?? 01 00", ADDRESS::Direction::DOWN, 100).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xEB }); // CAL --> NOP | Disable debugger check when miles searches for audio device to allow attaching the debugger to the game upon launch.
	FairFight_Init.Offset(0x0).FindPatternSelf("0F 87", ADDRESS::Direction::DOWN, 200).Patch({ 0x0F, 0x85 });                      // JA  --> JNZ | Prevent 'FairFight' anti-cheat from initializing on the server by comparing RAX against 0x0 instead. Init will crash since the plugins aren't shipped.
	SCR_BeginLoadingPlaque.Offset(0x1AD).FindPatternSelf("75 27", ADDRESS::Direction::DOWN).Patch({ 0xEB, 0x27 });                 // JNE --> JMP | Prevent connect command from crashing by invalid call to UI function.
	p_SQVM_CompileError.Offset(0x0).FindPatternSelf("41 B0 01", ADDRESS::Direction::DOWN, 400).Patch({ 0x41, 0xB0, 0x00 });        // MOV --> MOV | Set script error level to 0 (not severe): 'mov r8b, 0'.
	p_SQVM_CompileError.Offset(0xE0).FindPatternSelf("E8", ADDRESS::Direction::DOWN, 200).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | TODO: causes errors on client script error. Research required (same function as soft error but that one doesn't crash).
#else
	p_SQVM_CompileError.Offset(0xE0).FindPatternSelf("E8", ADDRESS::Direction::DOWN, 200).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | For dedicated we should not perform post-error events such as telemetry / showing 'COM_ExplainDisconnection' UI etc.
#endif // !DEDICATED

#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	p_CAI_NetworkManager__ShouldRebuild.Offset(0xA0).FindPatternSelf("FF ?? ?? ?? 00 00", ADDRESS::Direction::DOWN, 200).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Virtual call to restart when building AIN (which clears the AIN memory). Remove this once writing to file works.
	Detour_LevelInit.Offset(0x100).FindPatternSelf("74", ADDRESS::Direction::DOWN, 600).Patch({ 0xEB });                                                                // JE  --> JMP | Do while loop setting fields to -1 in navmesh is writing out of bounds (!TODO).
#endif
#ifndef GAMECLIENTONLY
	Server_S2C_CONNECT_1.Offset(0x7).Patch({ 0xEB }); // JZ --> JMP | Prevent entitlement check to kick player from server on S2C_CONNECT Packet if it does not match the servers one.
#endif // !GAMECLIENTONLY
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
#endif // GAMEDLL_S3
}
