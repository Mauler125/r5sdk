/*-----------------------------------------------------------------------------
 * _opcodes.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "common/opcodes.h"
//#include "common/netmessages.h"
//#include "engine/cmodel_bsp.h"
//#include "engine/host.h"
//#include "engine/host_cmd.h"
//#include "engine/gl_screen.h"
//#include "engine/gl_matsysiface.h"
//#include "engine/matsys_interface.h"
//#include "engine/modelloader.h"
#include "engine/server/sv_main.h"
//#include "engine/client/cl_main.h"
//#include "engine/client/client.h"
//#include "engine/client/clientstate.h"
//#include "engine/client/cdll_engine_int.h"
//#include "engine/sys_getmodes.h"
//#include "engine/sys_dll.h"
#ifndef CLIENT_DLL
#include "game/server/ai_networkmanager.h"
#include "game/server/detour_impl.h"
#endif // !CLIENT_DLL
//#include "rtech/rui/rui.h"
//#include "materialsystem/cmaterialsystem.h"
//#include "studiorender/studiorendercontext.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
//#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "codecs/miles/radshal_wasapi.h"
#endif // !DEDICATED

#ifdef DEDICATED
 //-------------------------------------------------------------------------
 // Purpose: change runtime behavior
 //-------------------------------------------------------------------------
void Dedicated_Init()
{
	*s_bIsDedicated = true;
//	//-------------------------------------------------------------------------
//	// CGAME
//	//-------------------------------------------------------------------------
//	{
//		p_CVideoMode_Common__CreateGameWindow.Offset(0x2C).Patch({ 0xE9, 0x9A, 0x00, 0x00, 0x00 });       // PUS --> XOR | Prevent ShowWindow and CreateGameWindow from being initialized (STGS RPak data type is registered here).
//		p_CVideoMode_Common__CreateWindowClass.Offset(0x0).Patch({ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 }); // FUN --> RET | Prevent CreateWindowClass from being initialized (returned true to satisfy condition that checks window handle).
//	}
//
//	//-------------------------------------------------------------------------
//	// CHLCLIENT
//	//-------------------------------------------------------------------------
//	{
//		p_CHLClient_LevelShutdown.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 }); // FUN --> RET | Return early in 'CHLClient::LevelShutdown()' during DLL shutdown.
//		p_CHLClient_HudProcessInput.Patch({ 0xC3 });                             // FUN --> RET | Return early in 'CHLClient::HudProcessInput()' to prevent infinite loop.
//
//		g_GameDll.FindPatternSIMD("41 85 C8 0F 84").Offset(0x40).Patch({ 0xEB, 0x23 }); // MOV --> JMP | Skip virtual call during settings layout parsing (S0/S1/S2/S3).
//	}
//
//	//-------------------------------------------------------------------------
//	// CCLIENTSTATE
//	//-------------------------------------------------------------------------
//	{
//		/*MOV EAX, 0*/
//		p_CClientState__RunFrame.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });   // FUN --> RET | Always return false for pending client snapshots (inline CClientState call in '_Host_RunFrame()')
//		p_CClientState__Disconnect.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 }); // FUN --> RET | Always return false for keeping client persistent data after disconnect (CLIENT ONLY).
//	}
//
//	//-------------------------------------------------------------------------
//	// CSOURCEAPPSYSTEMGROUP
//	//-------------------------------------------------------------------------
//	{
//		p_CSourceAppSystemGroup__Create.Offset(0x248).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x267).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | materials->Connect().
//		//p_CSourceAppSystemGroup__Create.Offset(0x286).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | mdlCache->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x2A5).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x2C4).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | avi->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x2E3).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Connect().
//		//p_CSourceAppSystemGroup__Create.Offset(0x302).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | dataCache->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x321).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matSystemSurface->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x340).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Connect().
//		p_CSourceAppSystemGroup__Create.Offset(0x35D).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | inputSystem->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x384).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | studioRender->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x391).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | avi->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x39E).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | bik->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x3AB).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | engineAPI->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x3F6).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | vgui->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x3E9).Patch({ 0x90, 0x90, 0x90 }); // CAL --> NOP | matEmbeddedPanel->Init().
//		p_CSourceAppSystemGroup__Create.Offset(0x3F9).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | EAC_ClientInterface_Init().
//	}
//
//	//-------------------------------------------------------------------------
//	// CMATERIALSYSTEM
//	//-------------------------------------------------------------------------
//	{
//		//gCMaterialSystem__MatsysMode_Init.Offset(0x22).Patch({ 0xEB, 0x66 });        // JE  --> JMP | Matsys mode init (CMaterialSystem). // TODO: Needed?
//		p_CMaterialSystem__Init.Offset(0x406).Patch({ 0xE9, 0x55, 0x05, 0x00, 0x00 }); // MOV --> JMP | Jump over material KeyValue definitions and 'CMatRenderContextBase::sm_RenderData([x])'.
//		p_InitMaterialSystem.Patch({ 0xC3 });                                          // FUN --> RET | Return early to prevent 'InitDebugMaterials' from being executed. // RESEARCH NEEDED.
//	}
//
//	//-------------------------------------------------------------------------
//	// CSHADERSYSTEM
//	//-------------------------------------------------------------------------
//	{
//		CShaderSystem__Init.Patch({ 0xC3 });                                         // FUN --> RET | Return early in 'CShaderSystem::Init()' to prevent initialization.
//	}
//
//	//-------------------------------------------------------------------------
//	// CSTUDIORENDERCONTEXT
//	//-------------------------------------------------------------------------
//	{
//		// Note: The registers here seems to contains pointers to material data and 'CMaterial' class methods when the shader system is initialized.
//		CStudioRenderContext__LoadModel.Offset(0x17D).Patch({ 0x90, 0x90, 0x90, 0x90 });             // MOV --> NOP | RAX + RCX are both nullptr.
//		CStudioRenderContext__LoadModel.Offset(0x181).Patch({ 0x90, 0x90, 0x90 });                   // MOV --> NOP | RCX is nullptr when trying to dereference.
//		CStudioRenderContext__LoadModel.Offset(0x184).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
//		CStudioRenderContext__LoadMaterials.Offset(0x28).Patch({ 0xE9, 0x80, 0x04, 0x00, 0x00 });    // FUN --> RET | 'CStudioRenderContext::LoadMaterials' is called virtually by the 'RMDL' streaming job.
//	}
//
//	//-------------------------------------------------------------------------
//	// CMODELLOADER
//	//-------------------------------------------------------------------------
//	{
//		p_CModelLoader__LoadModel.Offset(0x462).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });               // CAL --> NOP | Prevent call to 'CStudioRenderContext::LoadMaterials'.
//		p_CModelLoader__UnloadModel.Offset(0x129).Patch({ 0x90, 0x90, 0x90 });                         // MOV --> NOP | Virtual call to 'CShaderSystem' class method fails as RCX is nullptr.
//		p_CModelLoader__UnloadModel.Offset(0x12C).Patch({ 0x90, 0x90, 0x90 });                         // CAL --> NOP | Virtual call to 'CTexture' class member in RAX + 0x78 fails. Previous instruction could not dereference.
//		p_CModelLoader__Studio_LoadModel.Offset(0x325).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialSystem::FindMaterialEx' fails as RAX is nullptr.
//		p_CModelLoader__Studio_LoadModel.Offset(0x33D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
//		p_CModelLoader__Studio_LoadModel.Offset(0x359).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
//		p_CModelLoader__Studio_LoadModel.Offset(0x374).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
//		p_CModelLoader__Studio_LoadModel.Offset(0x38D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'ReturnZero' fails as RAX is nullptr.
//		p_CModelLoader__Studio_LoadModel.Offset(0x3A4).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });  // CAL --> NOP | Virtual call to 'CMaterialGlue' class method fails as RAX is nullptr.
//
//		p_CModelLoader__Map_LoadModelGuts.Offset(0x41).Patch({ 0xE9, 0x4F, 0x04, 0x00, 0x00 });        // JNE --> NOP | SKYLIGHTS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0x974).Patch({ 0x90, 0x90 });                         // JE  --> NOP | VERTNORMALS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xA55).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MATERIALSORTS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xA62).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MESHBOUNDS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xA83).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // CAL --> NOP | MESHVERTS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xAC0).Patch({ 0x90, 0x90 });                         // JE  --> NOP | INDICES.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xBF2).Patch({ 0x90, 0x90 });                         // JE  --> NOP | WORLDLIGHTS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xDA9).Patch({ 0x90, 0x90 });                         // JE  --> NOP | TWEAKLIGHTS.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0xEEB).Patch({ 0xE9, 0x3D, 0x01, 0x00, 0x00 });       // JLE --> JMP | Exception 0x57 in while trying to dereference [R15 + R14 *8 + 0x10].
//		p_CModelLoader__Map_LoadModelGuts.Offset(0x61B).Patch({ 0xE9, 0xE2, 0x02, 0x00, 0x00 });       // JZ  --> JMP | Prevent call to 'CMod_LoadTextures()'.
//		p_CModelLoader__Map_LoadModelGuts.Offset(0x1045).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });      // CAL --> NOP | Prevent call to 'Mod_LoadCubemapSamples()'.
//
//		p_BuildSpriteLoadName.Patch({ 0xC3 });                                                         // FUN --> RET | Return early in 'BuildSpriteLoadName()'.
//		p_GetSpriteInfo.Patch({ 0xC3 });                                                               // FUN --> RET | Return early in 'GetSpriteInfo()'.
//	}
//
//	//-------------------------------------------------------------------------
//	// CGAMESERVER
//	//-------------------------------------------------------------------------
//	{
//		p_CGameServer__SpawnServer.Offset(0x43).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Prevent call to unknown material/shader code.
//		p_CGameServer__SpawnServer.Offset(0x48).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | TODO: Research 'CIVDebugOverlay'.
//	}
//
//	//-------------------------------------------------------------------------
//	// CVGUI
//	//-------------------------------------------------------------------------
//	{
//		/*MOV EAX, 0*/
//		CVGui__RunFrame.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                   // FUN --> RET | 'CVGui::RunFrame()' gets called on DLL shutdown.
//	}
//	//-------------------------------------------------------------------------
//	// CRUI
//	//-------------------------------------------------------------------------
//	{
//		/*MOV EAX, 0*/
//		p_Rui_LoadAsset.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                    // FUN --> RET | Return early in RuiLoadAsset() to prevent error while attempting to load RUI assets after applying player settings.
//	}
//
//	//-------------------------------------------------------------------------
//	// CENGINEVGUI
//	//-------------------------------------------------------------------------
//	{
//		CEngineVGui__Shutdown.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                                  // FUN --> RET | Cannot shutdown CEngineVGui if its never initialized.
//		CEngineVGui__ActivateGameUI.FindPatternSelf("74 08", CMemory::Direction::DOWN).Patch({ 0x90, 0x90 }); // JZ  --> NOP | Remove condition to return early when engine attempts to activate UI on the server.
//	}
//
//	//-------------------------------------------------------------------------
//	// CENGINEVGUI
//	//-------------------------------------------------------------------------
//	{
//		CInputSystem__RunFrameIME.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });       // FUN --> RET | Return early in 'CInputSystem::RunFrameIME()'.
//	}
//
//	//-------------------------------------------------------------------------
//	// MM_HEARTBEAT
//	//-------------------------------------------------------------------------
//	{
//		MM_Heartbeat__ToString.Patch({ 0xC3 }); // SUB  --> RET | Return early in ListenServer HeartBeat.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: SYS_INITGAME
//	//-------------------------------------------------------------------------
//	{
//		Sys_InitGame.Offset(0x70).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // STZNZ --> NOP | Prevent 'bDedicated' from being set to false.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: HOST_INIT
//	//-------------------------------------------------------------------------
//	{
//		p_Host_Init.Offset(0xC2).Patch({ 0xEB, 0x34 });                    // CAL --> NOP | Disable 'vpk/client_common.bsp' loading.
//		p_Host_Init.Offset(0x182).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> JMP | Disable UI material asset initialization.
//		p_Host_Init.Offset(0x859).Patch({ 0xE9, 0x19, 0x04, 0x00, 0x00 }); // LEA --> RET | Disable 'client.dll' library initialization.
//		p_Host_Init.Offset(0xC77).Patch({ 0xE8, 0x44, 0xCF, 0xFF, 0xFF }); // CAL --> CAL | Disable user config loading and call entitlements.rson initialization instead.
//
//		gHost_Init_1.Offset(0x564).Patch({ 0xEB });                         // JNZ --> JMP | Skip chat room and discord presence thread creation [!TODO: set global boolean instead].
//		gHost_Init_1.Offset(0x609).Patch({ 0xEB, 0x2B });                   // JE  --> JMP | Skip client.dll 'Init_PostVideo()' validation code.
//		gHost_Init_1.Offset(0x621).Patch({ 0xEB, 0x0C });                   // JNE --> JMP | Skip client.dll 'Init_PostVideo()' validation code.
//		gHost_Init_1.Offset(0x658).Patch({ 0xE9, 0x8C, 0x00, 0x00, 0x00 }); // JE  --> JMP | Skip NULL call as client is never initialized.
//		gHost_Init_1.Offset(0x6E9).Patch({ 0xE9, 0xB0, 0x00, 0x00, 0x00 }); // JNE --> JMP | Skip shader preloading as cvar can't be checked due to client being NULL.
//
//		gHost_Init_2.Offset(0x26F).Patch({ 0xE9, 0x4D, 0x05, 0x00, 0x00 }); // JNE --> JMP | client.dll systems initialization.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: HOST_SHUTDOWN
//	//-------------------------------------------------------------------------
//	{
//#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
//		Host_Shutdown.Offset(0x1F0).FindPatternSelf("7E", CMemory::Direction::DOWN).Patch({ 0xE9, 0x01, 0x08, 0x00, 0x00 }); // JNE --> JMP | Jump over inline 'Host_ShutdownClient()' ('Host_ShutdownServer' in now inline with 'Host_Shutdown()')
//#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//		Host_Shutdown.Offset(0x1F0).FindPatternSelf("7E", CMemory::Direction::DOWN).Patch({ 0xE9, 0xF9, 0x04, 0x00, 0x00 }); // JNE --> JMP | Jump over inline 'Host_ShutdownClient()' ('Host_ShutdownServer' in now inline with 'Host_Shutdown()')
//#endif // 0x700
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: HOST_NEWGAME
//	//-------------------------------------------------------------------------
//	{
//		p_Host_NewGame.Offset(0x50).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Invalid CHLClient virtual call 'g_pHLClient->nullsub()'.
//		p_Host_NewGame.Offset(0x4E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });                        // CAL --> NOP | Matsys 'JT_HelpWithAnything()'.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: HOST_CHANGELEVEL
//	//-------------------------------------------------------------------------
//	{
//		p_Host_ChangeLevel.Offset(0x5D).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | Invalid CHLClient virtual call 'g_pHLClient->nullsub()'.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: _HOST_RUNFRAME
//	//-------------------------------------------------------------------------
//	{
//		p_Host_RunFrame.Offset(0xB85).Patch({ 0xEB, 0x6F });    // CMP --> JMP | Jump over inline '_Host_RunFrame_Client()'
//		p_Host_RunFrame_Render.Patch({ 0xC3 });                 // FUN --> RET | Extraneous function for Dedicated.
//		p_VCR_EnterPausedState.Patch({ 0xC3 });                 // FUN --> RET | Extraneous function for Dedicated.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: HOST_DISCONNECT
//	//-------------------------------------------------------------------------
//	{
//#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//		Host_Disconnect.Offset(0x4A).FindPatternSelf("FF 90 80", CMemory::Direction::DOWN, 300).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, }); // CAL --> RET | This seems to call 'CEngineVGui::GetGameUIInputContext()'.
//#endif
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: RTECH_GAME
//	//-------------------------------------------------------------------------
//	{
//#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//		p_CPakFile_LoadPak.Offset(0x890).FindPatternSelf("75", CMemory::Direction::DOWN, 200).Patch({ 0xEB });   // JNZ --> JMP | Disable error handling for missing streaming files on the server. The server does not need streamed data from the starpak files.
//#endif
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: EBISUSDK
//	//-------------------------------------------------------------------------
//	{
//		p_EbisuSDK_SetState.Offset(0x0).FindPatternSelf("0F 84", CMemory::Direction::DOWN).Patch({ 0x0F, 0x85 }); // JE  --> JNZ | Prevent EbisuSDK from initializing on the engine and server.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: FAIRFIGHT
//	//-------------------------------------------------------------------------
//	{
//		FairFight_Init.Offset(0x0).FindPatternSelf("0F 87", CMemory::Direction::DOWN, 200).Patch({ 0x0F, 0x85 }); // JA  --> JNZ | Prevent 'FairFight' anti-cheat from initializing on the server by comparing RAX against 0x0 instead. Init will crash since the plugins aren't shipped.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: PROP_STATIC
//	//-------------------------------------------------------------------------
//	{
//		// Note: At [14028F3B0 + 0x5C7] RSP seems to contain a block of pointers to data for the static prop rmdl in question. [RSP + 0x70] is a pointer to (what seems to be) shader/material data. The pointer will be NULL without a shader system.
//		p_BuildPropStaticFrustumCullMap.Offset(0x5E0).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 });       // MOV --> NOP | RSP + 0x70 is a nullptr which gets moved to R13, R13 gets used here resulting in exception 'C0000005'.
//		p_BuildPropStaticFrustumCullMap.Offset(0x5EB).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | RAX is nullptr during virtual call resulting in exception 'C0000005'.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: GL_SCREEN
//	//-------------------------------------------------------------------------
//	{
//		SCR_BeginLoadingPlaque.Patch({ 0xC3 });                                            // FUN --> RET | Return early to prevent execution of 'SCR_BeginLoadingPlaque()'.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: CL_CLEARSTATE
//	//-------------------------------------------------------------------------
//#if defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//	{
//		p_CL_ClearState.Offset(0x0).Patch({ 0xC3 });                                       // FUN --> RET | Invalid 'CL_ClearState()' call from Host_Shutdown causing segfault.
//	}
//#endif
//	//-------------------------------------------------------------------------
//	// RUNTIME: GAME_CFG
//	//-------------------------------------------------------------------------
//	p_UpdateMaterialSystemConfig.Offset(0x0).Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });// FUN --> RET | Return early to prevent the server from updating material system configurations.
//	p_UpdateCurrentVideoConfig.Offset(0x0).Patch({ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 });  // FUN --> RET | Return early to prevent the server from writing a videoconfig.txt file to the disk (overwriting the existing one).
//	p_HandleConfigFile.Offset(0x0).Patch({ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 });          // FUN --> RET | Return early to prevent the server from writing various input and ConVar config files to the disk (overwriting the existing one).
//	p_ResetPreviousGameState.Offset(0x0).Patch({ 0xC3 });                                  // FUN --> RET | Return early to prevent the server from writing a previousgamestate.txt file to the disk (overwriting the existing one).
//	p_LoadPlayerConfig.Offset(0x0).Patch({ 0xC3 });                                        // FUN --> RET | Return early to prevent the server from executing 'config_default_pc.cfg' (execPlayerConfig) and (only for >S3) running 'chat_wheel' code.
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: COMMUNITIES
//	//-------------------------------------------------------------------------
//	{
//		//GetEngineClientThread.Offset(0x0).Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });       // FUN --> RET | Return nullptr for mp_gamemode thread assignment during registration callback.
//	}
//
//	//-------------------------------------------------------------------------
//	// RUNTIME: MATCHMAKING
//	//-------------------------------------------------------------------------
//	{
//		MatchMaking_Frame.Patch({ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 });                   // FUN --> RET | Return early for 'MatchMaking_Frame()'.
//	}
//
//	{
//		CWin32Surface_initStaticData.Patch({ 0xC3 });                                      // FUN --> RET | Prevent 'CWin32Surface::initStaticData()' from being ran in CInit.
//#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
//		KeyboardLayout_Init.Patch({ 0xC3 });                                               // FUN --> RET | Prevent keyboard layout initialization for IME in CInit.
//#endif
//	}
}
#endif // DEDICATED

void RuntimePtc_Init() /* .TEXT */
{
#ifndef DEDICATED
	p_WASAPI_GetAudioDevice.Offset(0x410).FindPatternSelf("FF 15 ?? ?? 01 00", CMemory::Direction::DOWN, 100).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0xEB }); // CAL --> NOP | Disable debugger check when miles searches for audio device to allow attaching the debugger to the game upon launch.

	CMemory(v_SQVM_CompileError).Offset(0x0).FindPatternSelf("41 B0 01", CMemory::Direction::DOWN, 400).Patch({ 0x41, 0xB0, 0x00 });        // MOV --> MOV | Set script error level to 0 (not severe): 'mov r8b, 0'.
	CMemory(v_SQVM_CompileError).Offset(0xE0).FindPatternSelf("E8", CMemory::Direction::DOWN, 200).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | TODO: causes errors on client script error. Research required (same function as soft error but that one doesn't crash).
#else
	CMemory(v_SQVM_CompileError).Offset(0xE0).FindPatternSelf("E8", CMemory::Direction::DOWN, 200).Patch({ 0x90, 0x90, 0x90, 0x90, 0x90 }); // CAL --> NOP | For dedicated we should not perform post-error events such as telemetry / showing 'COM_ExplainDisconnection' UI etc.
#endif // !DEDICATED
}
