#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	SQVM_WarningFn originalSQVM_Warning = nullptr;
	SQVM_LoadRsonFn originalSQVM_LoadRson = nullptr;
	SQVM_LoadScriptFn originalSQVM_LoadScript = nullptr;
}

static std::ostringstream oss_print;
static auto ostream_sink_print = std::make_shared<spdlog::sinks::ostream_sink_st>(oss_print);

//---------------------------------------------------------------------------------
// Purpose: prints the output of each VM to the console
//---------------------------------------------------------------------------------
void* Hooks::SQVM_Print(void* sqvm, char* fmt, ...)
{
	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18);
	static bool initialized = false;

	static char buf[1024];
	static std::string vmType[3] = { "Script(S):", "Script(C):", "Script(U):" };

	static auto iconsole = spdlog::stdout_logger_mt("sqvm_print_iconsole"); // in-game console
	static auto wconsole = spdlog::stdout_logger_mt("sqvm_print_wconsole"); // windows console

	std::string vmStr = vmType[vmIdx].c_str();

	oss_print.str("");
	oss_print.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("ostream", ostream_sink_print);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v");
		wconsole->set_level(spdlog::level::debug);
		initialized = true;
	}

	va_list args;
	va_start(args, fmt);

	vsnprintf(buf, sizeof(buf), fmt, args);

	buf[sizeof(buf) - 1] = 0;
	va_end(args);

	vmStr.append(buf);

	iconsole->debug(vmStr);
	wconsole->debug(vmStr);

	std::string s = oss_print.str();
	const char* c = s.c_str();

	Items.push_back(Strdup((const char*)c));
	return NULL;
}

static std::ostringstream oss_warning;
static auto ostream_sink_warning = std::make_shared<spdlog::sinks::ostream_sink_st>(oss_warning);

__int64 Hooks::SQVM_Warning(void* sqvm, int a2, int a3, int* stringSize, void** string)
{
	__int64 result = originalSQVM_Warning(sqvm, a2, a3, stringSize, string);

	void* retaddr = _ReturnAddress(); // Get return address.

	if (retaddr != addr_SQVM_Warning_ReturnAddr) // Check if its SQVM_Warning calling.
		return result; // If not return.

	static bool initialized = false;
	static auto iconsole = spdlog::stdout_logger_mt("sqvm_warning_iconsole"); // in-game console
	static auto wconsole = spdlog::stdout_logger_mt("sqvm_warning_wconsole"); // windows console

	static std::string vmType[3] = { "Script(S) Warning:", "Script(C) Warning:", "Script(U) Warning:" };

	int vmIdx = *(int*)((std::uintptr_t)sqvm + 0x18); // Get vm index.

	std::string vmStr = vmType[vmIdx].c_str(); // Get string prefix for vm.

	oss_warning.str("");
	oss_warning.clear();

	if (!initialized)
	{
		iconsole = std::make_shared<spdlog::logger>("ostream", ostream_sink_warning);
		iconsole->set_pattern("[%S.%e] %v");
		iconsole->set_level(spdlog::level::debug);
		wconsole->set_pattern("[%S.%e] %v\n");
		wconsole->set_level(spdlog::level::debug);
		initialized = true;
	}

	std::string stringConstructor((char*)*string, *stringSize); // Get string from memory via std::string constructor.
	vmStr.append(stringConstructor);

	iconsole->debug(vmStr.c_str());
	wconsole->debug(vmStr.c_str());

	std::string s = oss_warning.str();
	const char* c = s.c_str();

	Items.push_back(Strdup((const char*)c));

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: loads the include file from the mods directory
//---------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------
// Purpose: loads the script file from the mods directory
//---------------------------------------------------------------------------------
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