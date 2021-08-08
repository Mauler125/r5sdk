#include "pch.h"
#include "hooks.h"
namespace Hooks
{
	FileSystemWarningFn originalFileSystemWarning = nullptr;
}

void Hooks::FileSystemWarning(void* thisptr, FileWarningLevel_t level, const char* fmt, ...)
{
//  How you call original functions, you dont need it here.
//	originalFileSystemWarning(thisptr, level, fmt, ...);
}