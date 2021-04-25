#include <string>
#include <stdarg.h>

#include <Windows.h>
#include <detours.h>

#include "utilities.h"
#include "hooks.h"
#include "patterns.h"
#include "structs.h"

//---------------------------------------------------------------------------------
// Engine Hooks
//---------------------------------------------------------------------------------

bool Hook_NET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = NET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		int i = NULL;
		netpacket_t* pkt = (netpacket_t*)inpacket;

		// Log received packet data
		HexDump("", "", "", 0, &pkt->data[i], pkt->wiresize);
	}

	return result;
}

unsigned int Hook_NET_SendDatagram(SOCKET s, const char* buf, int len, int flags)
{
	unsigned int result = NET_SendDatagram(s, buf, len, flags);
	if (result)
	{
		// Log transmitted packet data
		HexDump("", "", "", 0, buf, len);
	}

	return result;
}

void* Hook_SQVM_Print(void* sqvm, char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	return NULL;
}

bool Hook_SQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag)
{
	char filepath[MAX_PATH] = { 0 };
	sprintf_s(filepath, MAX_PATH, "platform\\%s", script_path);

    // Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
	{
		if (filepath[i] == '/')
		{
			filepath[i] = '\\';
		}
	}

	printf(" + Loading SQVM Script '%s' ...\n", filepath);
	if (FileExists(filepath) && SQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true; // Redirect to disk worked / script exists on disk..
	}
	
	printf(" |- FAILED, loading from SearchPath / VPK...\n");
	return SQVM_LoadScript(sqvm, script_path, script_name, flag);
}

//---------------------------------------------------------------------------------
// Hook Management
//---------------------------------------------------------------------------------

void InstallHooks()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook Functions
	DetourAttach((LPVOID*)&SQVM_Print, &Hook_SQVM_Print);
	DetourAttach((LPVOID*)&SQVM_LoadScript, &Hook_SQVM_LoadScript);

	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}
}

void RemoveHooks()
{
	// Begin the detour transaction, to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook Functions
	DetourDetach((LPVOID*)&SQVM_Print, &Hook_SQVM_Print);
	DetourDetach((LPVOID*)&SQVM_LoadScript, &Hook_SQVM_LoadScript);
	DetourDetach((LPVOID*)&ConVar_IsFlagSet, &Hook_ConVar_IsFlagSet);
	DetourDetach((LPVOID*)&ConCommand_IsFlagSet, &Hook_ConCommand_IsFlagSet);
	DetourDetach((LPVOID*)&NET_SendDatagram, &Hook_NET_SendDatagram);
	DetourDetach((LPVOID*)&NET_ReceiveDatagram, &Hook_NET_ReceiveDatagram);

	// Commit the transaction
	DetourTransactionCommit();
}

void ToggleDevCommands()
{
	static bool g_dev = false;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!g_dev)
	{
		DetourAttach((LPVOID*)&ConVar_IsFlagSet, &Hook_ConVar_IsFlagSet);
		DetourAttach((LPVOID*)&ConCommand_IsFlagSet, &Hook_ConCommand_IsFlagSet);
	}
	else
	{
		DetourDetach((LPVOID*)&ConVar_IsFlagSet, &Hook_ConVar_IsFlagSet);
		DetourDetach((LPVOID*)&ConCommand_IsFlagSet, &Hook_ConCommand_IsFlagSet);
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	g_dev = !g_dev;
}

void ToggleNetHooks()
{
	static bool g_net = false;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (!g_net)
	{
		DetourAttach((LPVOID*)&NET_SendDatagram, &Hook_NET_SendDatagram);
		DetourAttach((LPVOID*)&NET_ReceiveDatagram, &Hook_NET_ReceiveDatagram);
	}
	else
	{
		DetourDetach((LPVOID*)&NET_SendDatagram, &Hook_NET_SendDatagram);
		DetourDetach((LPVOID*)&NET_ReceiveDatagram, &Hook_NET_ReceiveDatagram);
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	g_net = !g_net;
}