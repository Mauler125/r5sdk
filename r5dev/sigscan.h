#pragma once
#include <Psapi.h>

class SigScan
{
public:
	// For getting information about the executing module
	MODULEINFO GetModuleInfo(const char *szModule)
	{
		MODULEINFO modinfo = { 0 };
		HMODULE hModule = GetModuleHandle(szModule);
		if (hModule == 0)
		{
			return modinfo;
		}
		GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
		return modinfo;
	}

	// For finding a signature/pattern in memory of the game process
	LONGLONG FindPattern(const char *module, const char *pattern, const char *mask)
	{
		MODULEINFO mInfo = GetModuleInfo(module);
		LONGLONG base = (LONGLONG)mInfo.lpBaseOfDll;
		LONGLONG size = (LONGLONG)mInfo.SizeOfImage;
		LONGLONG patternLength = (LONGLONG)strlen(mask);

		for (LONGLONG i = 0; i < size - patternLength; i++)
		{
			bool found = true;
			for (LONGLONG j = 0; j < patternLength; j++)
			{
				found &= mask[j] == '?' || pattern[j] == *(const char*)(base + i + j);
			}
			if (found)
			{
				return base + i;
			}
		}

		return NULL;
	}
};