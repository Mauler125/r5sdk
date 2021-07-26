#include "pch.h"
#include "sqvm.h"

//---------------------------------------------------------------------------------
// Print the output of the VM.
// TODO: separate SV CL and UI
//---------------------------------------------------------------------------------
void* HSQVM_PrintFunc(void* sqvm, char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	return NULL;
}

//---------------------------------------------------------------------------------
// Load the include file from the mods directory
//---------------------------------------------------------------------------------
__int64 HSQVM_LoadRson(const char* rson_name)
{
	char filepath[MAX_PATH] = { 0 };
	sprintf_s(filepath, MAX_PATH, "platform\\%s", rson_name);

	// Flip forward slashes in filepath to windows-style backslash
	for (int i = 0; i < strlen(filepath); i++)
	{
		if (filepath[i] == '/')
		{
			filepath[i] = '\\';
		}
	}

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

//---------------------------------------------------------------------------------
// Load the script file from the mods directory
//---------------------------------------------------------------------------------
bool HSQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag)
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
	if (g_bDebugLoading)
	{
		printf(" [+] Loading SQVM Script '%s' ...\n", filepath);
	}

	// Returns true if the script exists on the disk
	if (FileExists(filepath) && org_SQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true;
	}
	if (g_bDebugLoading)
	{
		printf(" [!] FAILED. Try SP / VPK for '%s'\n", filepath);
	}

	///////////////////////////////////////////////////////////////////////////////
	return org_SQVM_LoadScript(sqvm, script_path, script_name, flag);
}

void AttachSQVMHooks()
{
	DetourAttach((LPVOID*)&org_SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourAttach((LPVOID*)&org_SQVM_LoadRson, &HSQVM_LoadRson);
	DetourAttach((LPVOID*)&org_SQVM_LoadScript, &HSQVM_LoadScript);
}

void DetachSQVMHooks()
{
	DetourDetach((LPVOID*)&org_SQVM_PrintFunc, &HSQVM_PrintFunc);
	DetourDetach((LPVOID*)&org_SQVM_LoadRson, &HSQVM_LoadRson);
	DetourDetach((LPVOID*)&org_SQVM_LoadScript, &HSQVM_LoadScript);
}