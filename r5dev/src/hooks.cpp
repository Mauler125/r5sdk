#include "pch.h"
#include "patterns.h"
#include "structs.h"
#include "overlay.h"
#include "hooks.h"
#include "gameclasses.h"

//#################################################################################
// NETCHANNEL HOOKS
//#################################################################################

bool HNET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = org_NET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		int i = NULL;
		netpacket_t* pkt = (netpacket_t*)inpacket;

		///////////////////////////////////////////////////////////////////////////
		// Log received packet data
		HexDump("[+] NET_ReceiveDatagram", 0, &pkt->data[i], pkt->wiresize);
	}

	return result;
}

unsigned int HNET_SendDatagram(SOCKET s, const char* buf, int len, int flags)
{
	unsigned int result = org_NET_SendDatagram(s, buf, len, flags);
	if (result)
	{
		///////////////////////////////////////////////////////////////////////////
		// Log transmitted packet data
		HexDump("[+] NET_SendDatagram",  0, buf, len);
	}

	return result;
}

//#################################################################################
// CHLCLIENT HOOKS
//#################################################################################

void __fastcall HCHLClient__FrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage) /* __fastcall so we can make sure first argument will be RCX and second RDX. */
{
	switch (curStage)
	{
	case FRAME_START: // FrameStageNotify gets called every frame by CEngine::Frame with the stage being FRAME_START. We can use this to check/set global variables.
	{
		if (!GameGlobals::IsInitialized)
			GameGlobals::InitGameGlobals();

		break;
	}
	default:
		break;
	}

	org_CHLClient_FrameStageNotify(rcx, curStage);
}

//#################################################################################
// SQUIRRELVM HOOKS
//#################################################################################

void* HSQVM_Print(void* sqvm, char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	Items.push_back(Strdup(buf));
	return NULL;
}

__int64 HSQVM_LoadRson(const char* rson_name)
{
	char filepath[MAX_PATH] = { 0 };
	sprintf_s(filepath, MAX_PATH, "platform\\%s", rson_name);

	///////////////////////////////////////////////////////////////////////////////
	// Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
	{
		if (filepath[i] == '/')
		{
			filepath[i] = '\\';
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// Returns the new path if the rson exists on the disk
	if (FileExists(filepath) && org_SQVM_LoadRson(rson_name))
	{
		printf("\n");
		printf("##################################################\n");
		printf("] '%s'\n", filepath);
		printf("##################################################\n");
		printf("\n");
		return org_SQVM_LoadRson(filepath);
	}

	printf("\n");
	printf("##################################################\n");
	printf("] '%s'\n", rson_name);
	printf("##################################################\n");
	printf("\n");
	return org_SQVM_LoadRson(rson_name);
}

bool HSQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag)
{
	char filepath[MAX_PATH] = { 0 };
	sprintf_s(filepath, MAX_PATH, "platform\\%s", script_path);

	///////////////////////////////////////////////////////////////////////////////
	// Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
	{
		if (filepath[i] == '/')
		{
			filepath[i] = '\\';
		}
	}
	if (g_bDebugLoading)
	{
		printf(" [+] Loading SQVM Script '%s' ...\n", filepath);
	}
	///////////////////////////////////////////////////////////////////////////////
	// Returns true if the script exists on the disk
	if (FileExists(filepath) && org_SQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true;
	}
	if (g_bDebugLoading)
	{
		printf(" [!] FAILED. Try SP / VPK for '%s'\n", filepath);
	}
	return org_SQVM_LoadScript(sqvm, script_path, script_name, flag);
}

//#################################################################################
// UTILITY HOOKS
//#################################################################################

int HMSG_EngineError(char* fmt, va_list args)
{
	printf("\nENGINE ERROR #####################################\n");
	vprintf(fmt, args);
	return org_MSG_EngineError(fmt, args);
}

// TODO: turn this into a playerstruct constructor if it ever becomes necessary
bool HCVEngineClient_IsPersistenceDataAvailable(__int64 thisptr, int client)
{
	static bool isPersistenceVarSet[256];

	// TODO: Maybe not hardcode
	std::uintptr_t playerStructBase = 0x16073B200;
	std::uintptr_t playerStructSize = 0x4A4C0;
	std::uintptr_t persistenceVar = 0x5BC;

	std::uintptr_t targetPlayerStruct = playerStructBase + client * playerStructSize;

	*(char*)(targetPlayerStruct + persistenceVar) = (char)0x5;

	if (!isPersistenceVarSet[client])
	{
		printf("\n");
		printf("##################################################\n");
		printf("] SETTING PERSISTENCE VAR FOR CLIENT #%d\n", client);
		printf("##################################################\n");
		printf("\n");
		isPersistenceVarSet[client] = true;
	}

	return org_CVEngineServer_IsPersistenceDataAvailable(thisptr, client);
}

//#################################################################################
// MANAGEMENT
//#################################################################################

void InstallENHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////////
	// Hook Squirrel functions
	DetourAttach((LPVOID*)&org_SQVM_Print, &HSQVM_Print);
	DetourAttach((LPVOID*)&org_SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&org_SQVM_LoadScript, &HSQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Hook Game Functions
	DetourAttach((LPVOID*)&org_CHLClient_FrameStageNotify, &HCHLClient__FrameStageNotify);

	///////////////////////////////////////////////////////////////////////////////
	// Hook Utility functions
	DetourAttach((LPVOID*)&org_MSG_EngineError, &HMSG_EngineError);
	DetourAttach((LPVOID*)&org_CVEngineServer_IsPersistenceDataAvailable, &HCVEngineClient_IsPersistenceDataAvailable);

	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}
}

void RemoveENHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction, to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Squirrel functions
	DetourDetach((LPVOID*)&org_SQVM_Print, &HSQVM_Print);
	DetourDetach((LPVOID*)&org_SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&org_SQVM_LoadScript, &HSQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Game Functions
	DetourDetach((LPVOID*)&org_CHLClient_FrameStageNotify, &HCHLClient__FrameStageNotify);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Netchan functions
	DetourDetach((LPVOID*)&org_NET_SendDatagram, &HNET_SendDatagram);
	DetourDetach((LPVOID*)&org_NET_ReceiveDatagram, &HNET_ReceiveDatagram);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Utility functions
	DetourDetach((LPVOID*)&org_MSG_EngineError, &HMSG_EngineError);
	DetourDetach((LPVOID*)&org_CVEngineServer_IsPersistenceDataAvailable, &HCVEngineClient_IsPersistenceDataAvailable);

	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	DetourTransactionCommit();
}

//#################################################################################
// TOGGLES
//#################################################################################

void ToggleNetHooks()
{
	static bool g_net = false;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!g_net)
	{
		DetourAttach((LPVOID*)&org_NET_SendDatagram, &HNET_SendDatagram);
		DetourAttach((LPVOID*)&org_NET_ReceiveDatagram, &HNET_ReceiveDatagram);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| NETCHANNEL TRACE ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		DetourDetach((LPVOID*)&org_NET_SendDatagram, &HNET_SendDatagram);
		DetourDetach((LPVOID*)&org_NET_ReceiveDatagram, &HNET_ReceiveDatagram);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>| NETCHANNEL TRACE DEACTIVATED |<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	g_net = !g_net;
}
