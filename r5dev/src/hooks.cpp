#include <string>

#include <Windows.h>
#include <detours.h>

#include "patterns.h"
#include "utility.h"
#include "structs.h"
#include "overlay.h"
#include "hooks.h"
#include "gameclasses.h"

//#################################################################################
// NETCHANNEL HOOKS
//#################################################################################

bool HNET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = NET_ReceiveDatagram(sock, inpacket, raw);
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
	unsigned int result = NET_SendDatagram(s, buf, len, flags);
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
	static CHostState* HostState = reinterpret_cast<CHostState*>(0x141736120);

	switch (curStage)
	{
	case FRAME_START: // FrameStageNotify gets called every frame by CEngine::Frame with the stage being FRAME_START. We can use this to check/set global variables.
	{
		if (HostState->m_bWaitingForConnection) // Easy way to check if we are DEDI.
		{
		//	printf("AWAITING CONNECTION\n");
		}

		break;
	}
	default:
		break;
	}

	CHLClient_FrameStageNotify(rcx, curStage);
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
	if (FileExists(filepath) && SQVM_LoadRson(rson_name))
	{
		printf("\n");
		printf("##################################################\n");
		printf("] '%s'\n", filepath);
		printf("##################################################\n");
		printf("\n");
		return SQVM_LoadRson(filepath);
	}
	else
	{
		printf("\n");
		printf("##################################################\n");
		printf("] '%s'\n", rson_name);
		printf("##################################################\n");
		printf("\n");
		return SQVM_LoadRson(rson_name);
	}
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
	if (FileExists(filepath) && SQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true;
	}
	if (g_bDebugLoading)
	{
		printf(" [!] FAILED. Try SP / VPK for '%s'\n", filepath);
	}
	return SQVM_LoadScript(sqvm, script_path, script_name, flag);
}

//#################################################################################
// UTILITY HOOKS
//#################################################################################

int HMSG_EngineError(char* fmt, va_list args)
{
	char buf[1024];
	printf("ENGINE ERROR #####################################\n");
	vprintf(fmt, args);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	Items.push_back(Strdup(buf));
	return MSG_EngineError(fmt, args);
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
	DetourAttach((LPVOID*)&SQVM_Print, &HSQVM_Print);
	DetourAttach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Hook Game Functions
	DetourAttach((LPVOID*)&CHLClient_FrameStageNotify, &HCHLClient__FrameStageNotify);

	///////////////////////////////////////////////////////////////////////////////
	// Hook Utility functions
	DetourAttach((LPVOID*)&MSG_EngineError, &HMSG_EngineError);

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
	DetourDetach((LPVOID*)&SQVM_Print, &HSQVM_Print);
	DetourDetach((LPVOID*)&SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&SQVM_LoadScript, &HSQVM_LoadScript);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Game Functions
	DetourDetach((LPVOID*)&CHLClient_FrameStageNotify, &HCHLClient__FrameStageNotify);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Netchan functions
	DetourDetach((LPVOID*)&NET_SendDatagram, &HNET_SendDatagram);
	DetourDetach((LPVOID*)&NET_ReceiveDatagram, &HNET_ReceiveDatagram);

	///////////////////////////////////////////////////////////////////////////////
	// Unhook Utility functions
	DetourDetach((LPVOID*)&MSG_EngineError, &HMSG_EngineError);

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
		DetourAttach((LPVOID*)&NET_SendDatagram, &HNET_SendDatagram);
		DetourAttach((LPVOID*)&NET_ReceiveDatagram, &HNET_ReceiveDatagram);
		printf("\n");
		printf("+--------------------------------------------------------+\n");
		printf("|>>>>>>>>>>>>>| NETCHANNEL TRACE ACTIVATED |<<<<<<<<<<<<<|\n");
		printf("+--------------------------------------------------------+\n");
		printf("\n");
	}
	else
	{
		DetourDetach((LPVOID*)&NET_SendDatagram, &HNET_SendDatagram);
		DetourDetach((LPVOID*)&NET_ReceiveDatagram, &HNET_ReceiveDatagram);
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
