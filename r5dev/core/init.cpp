//=============================================================================//
//
// Purpose: Main systems initialization file
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/init.h"
#include "tier0/ConCommand.h"
#include "tier0/completion.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "vpc/IAppSystem.h"
#include "vpc/keyvalues.h"
#include "vpc/basefilesystem.h"
#include "vpc/keyvalues.h"
#include "common/opcodes.h"
#include "launcher/IApplication.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "milessdk/win64_rrthreads.h"
#endif // !DEDICATED
#include "vphysics/QHull.h"
#include "bsplib/bsplib.h"
#ifndef DEDICATED
#include "materialsystem/materialsystem.h"
#include "vgui/CEngineVGui.h"
#include "vgui/vgui_fpspanel.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "client/cdll_engine_int.h"
#endif // !DEDICATED
#include "client/client.h"
#include "client/IVEngineClient.h"
#include "server/server.h"
#include "server/IVEngineServer.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqvm.h"
#include "rtech/rtech_game.h"
#include "rtech/stryder.h"
#include "engine/baseclient.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/net_chan.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/sys_utils.h"
#ifndef DEDICATED
#include "engine/debugoverlay.h"
#include "inputsystem/inputsystem.h"
#include "windows/id3dx.h"
#endif // !DEDICATED


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// ██╗███╗   ██╗██╗████████╗██╗ █████╗ ██╗     ██╗███████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
// ██║████╗  ██║██║╚══██╔══╝██║██╔══██╗██║     ██║╚══███╔╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
// ██║██╔██╗ ██║██║   ██║   ██║███████║██║     ██║  ███╔╝ ███████║   ██║   ██║██║   ██║██╔██╗ ██║
// ██║██║╚██╗██║██║   ██║   ██║██╔══██║██║     ██║ ███╔╝  ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
// ██║██║ ╚████║██║   ██║   ██║██║  ██║███████╗██║███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
// ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void Systems_Init()
{
	// Begin the detour transaction to hook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	IApplication_Attach();
	CBaseClient_Attach();
	CBaseFileSystem_Attach();

	QHull_Attach();
	//BspLib_Attach();

#ifndef DEDICATED
	CEngineVGui_Attach();
	CFPSPanel_Attach();
	CHLClient_Attach();
#endif // !DEDICATED

#ifdef GAMEDLL_S3
	CServer_Attach(); // S1 and S2 CServer functions require work.
#endif // GAMEDLL_S3

#ifdef DEDICATED
	CHostState_Attach(); // Dedicated only for now until backwards compatible with S1.
#endif // DEDICATED

	CNetChan_Attach();
	ConCommand_Attach();
	IConVar_Attach();
	CKeyValueSystem_Attach();
	IVEngineServer_Attach();
	SQAPI_Attach();
	SQVM_Attach();

	RTech_Game_Attach();

	SysDll_Attach();
	SysUtils_Attach();

	// Patch instructions
	RuntimePtc_Init();

	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	IConVar_InitConVar();

#ifdef DEDICATED
	Dedicated_Init();
#endif // DEDICATED

}

//////////////////////////////////////////////////////////////////////////
//
// ███████╗██╗  ██╗██╗   ██╗████████╗██████╗  ██████╗ ██╗    ██╗███╗   ██╗
// ██╔════╝██║  ██║██║   ██║╚══██╔══╝██╔══██╗██╔═══██╗██║    ██║████╗  ██║
// ███████╗███████║██║   ██║   ██║   ██║  ██║██║   ██║██║ █╗ ██║██╔██╗ ██║
// ╚════██║██╔══██║██║   ██║   ██║   ██║  ██║██║   ██║██║███╗██║██║╚██╗██║
// ███████║██║  ██║╚██████╔╝   ██║   ██████╔╝╚██████╔╝╚███╔███╔╝██║ ╚████║
// ╚══════╝╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═════╝  ╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═══╝
//
//////////////////////////////////////////////////////////////////////////

void Systems_Shutdown()
{
	// Begin the detour transaction to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook functions
	IApplication_Detach();
	CBaseClient_Detach();
	CBaseFileSystem_Detach();

	QHull_Detach();
	//BspLib_Detach();

#ifndef DEDICATED
	CEngineVGui_Detach();
	CFPSPanel_Detach();
	CHLClient_Detach();
#endif // !DEDICATED

#ifdef GAMEDLL_S3
	CServer_Detach(); // S1 and S2 CServer functions require work.
#endif // GAMEDLL_S3

#ifdef DEDICATED
	CHostState_Detach(); // Dedicated only for now until backwards compatible with S1.
#endif // DEDICATED

	CNetChan_Detach();
	ConCommand_Detach();
	IConVar_Detach();
	CKeyValueSystem_Detach();
	IVEngineServer_Detach();
	SQAPI_Detach();
	SQVM_Detach();

	RTech_Game_Detach();

	SysDll_Detach();
	SysUtils_Detach();

	// Commit the transaction
	DetourTransactionCommit();
}

//////////////////////////////////////////////////////////
//
// ██████╗ ███████╗███████╗██╗   ██╗██╗  ████████╗███████╗
// ██╔══██╗██╔════╝██╔════╝██║   ██║██║  ╚══██╔══╝██╔════╝
// ██████╔╝█████╗  ███████╗██║   ██║██║     ██║   ███████╗
// ██╔══██╗██╔══╝  ╚════██║██║   ██║██║     ██║   ╚════██║
// ██║  ██║███████╗███████║╚██████╔╝███████╗██║   ███████║
// ╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝ ╚══════╝╚═╝   ╚══════╝
//
//////////////////////////////////////////////////////////

void PrintHAddress() // Test the sigscan results
{
	std::cout << "+----------------------------------------------------------------+" << std::endl;
	for (IDetour* pdetour : vdetour)
	{
		pdetour->debugp();
	}
}
