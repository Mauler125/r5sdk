#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	MSG_EngineErrorFn originalMSG_EngineError = nullptr;
}

int Hooks::MSG_EngineError(char* fmt, va_list args)
{
	std::cout << "\nENGINE ERROR #####################################\n";
	vprintf(fmt, args);

	return originalMSG_EngineError(fmt, args);
}