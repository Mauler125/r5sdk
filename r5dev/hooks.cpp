#include <string>
#include <stdarg.h>

#include <Windows.h>
#include <detours.h>

#include "utilities.h"
#include "hooks.h"

// TODO pretty sloppy, all hooks / eng offets or patterns should probably go in their own header..

void* SQVM_Print = (void*)0x140B40770;
void* Hook_SQVM_Print(void* sqvm, char* fmt, ...);

bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))0x140B3E880;
bool Hook_SQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag);

bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))0x1401F9680;
bool Hook_NET_ReceiveDatagram(int sock, void* inpacket, bool raw);

unsigned int (*NET_SendDatagram)(SOCKET s, const char* ptxt, int len, void* netchan_maybe, bool raw) = (unsigned int (*)(SOCKET, const char*, int, void*, bool))0x1401F8220;
unsigned int Hook_NET_SendDatagram(SOCKET s, const char* ptxt, int len, void* netchan_maybe, bool raw);

//
// TODO move to utils
// 

typedef unsigned __int64 QWORD;

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//
// Engine Hooks
//

struct __declspec(align(8)) netpacket_t
{
	DWORD family_maybe;
	sockaddr_in sin;
	WORD sin_port;
	BYTE gap16;
	BYTE byte17;
	DWORD source;
	double received;
	unsigned __int8* data;
	QWORD label;
	BYTE byte38;
	QWORD qword40;
	QWORD qword48;
	BYTE gap50[8];
	QWORD qword58;
	QWORD qword60;
	QWORD qword68;
	int less_than_12;
	DWORD wiresize;
	BYTE gap78[8];
	QWORD qword80;
};

bool Hook_NET_ReceiveDatagram(int sock, void* inpacket, bool raw)
{
	bool result = NET_ReceiveDatagram(sock, inpacket, raw);
	if (result)
	{
		netpacket_t* pkt = (netpacket_t *)inpacket;

		// TODO: print the rest of the data..
		printf("Got packet! Len %u\n -- ", pkt->wiresize);
		for (int i = 0; i < 16 && i < pkt->wiresize; i++)
		{
			printf("%02X ", pkt->data[i]);
		}

		printf("\n");
	}
	return result;
}

unsigned int Hook_NET_SendDatagram(SOCKET s, const char* ptxt, int len, void* netchan_maybe, bool raw)
{
	// TODO print ptxt[...] up to len bytes
	printf("Sending packet! %u bytes @ %p\n", len, ptxt);
	return NET_SendDatagram(s, ptxt, len, netchan_maybe, raw);
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
	char filepath[MAX_PATH] = {0};
	sprintf_s(filepath, MAX_PATH, "C:\\r5\\%s", script_path);

    // flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
		if (filepath[i] == '/')
			filepath[i] = '\\';

	printf(" + Loading SQVM Script '%s' ...\n", filepath);
	if (FileExists(filepath) && SQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true; // rediret to disk worked / script exists on disk..
	}
	
	printf(" |- FAILED, loading normally / from VPK...\n");
	return SQVM_LoadScript(sqvm, script_path, script_name, flag);
}

//
// Hook Management
//

void InstallHooks()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook Functions
	DetourAttach(&SQVM_Print, &Hook_SQVM_Print);
	DetourAttach((PVOID*)&SQVM_LoadScript, &Hook_SQVM_LoadScript);

	// TODO these might be fucked right now so they're disabled
	//DetourAttach((PVOID*)&NET_SendDatagram, &Hook_NET_SendDatagram);
	//DetourAttach((PVOID*)&NET_ReceiveDatagram, &Hook_NET_ReceiveDatagram);

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

	//DetourDetach(&SQVMPrint, &Hook_SQVMPrint);

	DetourTransactionCommit();
}