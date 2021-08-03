#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	SQVM_LoadRsonFn originalSQVM_LoadRson = nullptr;
	SQVM_LoadScriptFn originalSQVM_LoadScript = nullptr;
}

std::string prefixes[3] = { "Script(S)", "Script(C)", "Script(U)" };

void* Hooks::SQVM_Print(void* sqvm, char* fmt, ...)
{
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
	char buf[1024];
	va_list args;
	va_start(args, fmt);

	// external console
	printf("%s:", prefixes[vmIdx].c_str());
	vprintf(fmt, args);

	// imgui console
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;

	va_end(args);

	Items.push_back(Strdup(buf));
	return NULL;
}

__int64 Hooks::SQVM_LoadRson(const char* rson_name)
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
	if (FileExists(filepath) && originalSQVM_LoadRson(rson_name))
	{
		printf("\n");
		printf("##################################################\n");
		printf("] '%s'\n", filepath);
		printf("##################################################\n");
		printf("\n");

		return originalSQVM_LoadRson(filepath);
	}

	printf("\n");
	printf("##################################################\n");
	printf("] '%s'\n", rson_name);
	printf("##################################################\n");
	printf("\n");

	return originalSQVM_LoadRson(rson_name);
}

bool Hooks::SQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag)
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
	if (FileExists(filepath) && originalSQVM_LoadScript(sqvm, filepath, script_name, flag))
	{
		return true;
	}
	if (g_bDebugLoading)
	{
		printf(" [!] FAILED. Try SP / VPK for '%s'\n", filepath);
	}

	return originalSQVM_LoadScript(sqvm, script_path, script_name, flag);
}