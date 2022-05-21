#include "stdafx.h"
#include "Environment.h"
#include "Path.h"
#include <shellapi.h>

namespace System
{
	const String Environment::NewLine = "\r\n";

	void Environment::Exit(int32_t ExitCode)
	{
		std::exit(ExitCode);
	}

	String Environment::GetFolderPath(SpecialFolder Folder)
	{
		char Buffer[MAX_PATH + 1]{};
		auto Result = SHGetFolderPathA(NULL, (int)Folder, NULL, SHGFP_TYPE_CURRENT, Buffer);

		if (Result < 0)
			return "";

		return String(Buffer);
	}

	String Environment::GetApplicationPath()
	{
		char Buffer[MAX_PATH + 1]{};
		auto mResult = GetModuleFileNameA(NULL, Buffer, MAX_PATH);

		if (mResult != 0)
			return IO::Path::GetDirectoryName(Buffer);

		return "";
	}

	String Environment::GetApplication()
	{
		char Buffer[MAX_PATH + 1]{};
		auto mResult = GetModuleFileNameA(NULL, Buffer, MAX_PATH);

		if (mResult != 0)
			return Buffer;

		return "";
	}

	String Environment::GetCommandLine()
	{
		return String(GetCommandLineA());
	}

	List<String> Environment::GetCommandLineArgs()
	{
		auto Result = List<String>();

		int nArgs = 0;
		auto szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if (szArgList == nullptr)
			return Result;

		for (int i = 0; i < nArgs; i++)
			Result.EmplaceBack(std::move(WString(szArgList[i]).ToString()));

		LocalFree(szArgList);
		return Result;
	}

	String Environment::GetUserName()
	{
		char Buffer[1024]{};
		DWORD BufferSize = 1024;
		GetUserNameA(Buffer, &BufferSize);

		return String(Buffer);
	}

	String Environment::GetComputerName()
	{
		char Buffer[MAX_COMPUTERNAME_LENGTH + 1]{};
		DWORD BufferSize = MAX_COMPUTERNAME_LENGTH;
		GetComputerNameA(Buffer, &BufferSize);

		return String(Buffer);
	}

	uint64_t Environment::GetTickCount()
	{
		return GetTickCount64();
	}

	void Environment::SetEnvironmentVariable(const String& Key, const String& Value)
	{
		if (Key.Length() == 0 || Value.Length() == 0)
			return;

		SetEnvironmentVariableA((const char*)Key, (const char*)Value);
	}

	String Environment::GetEnvironmentVariable(const String& Key)
	{
		char Buffer[1024]{};
		GetEnvironmentVariableA((const char*)Key, Buffer, 1024);

		return String(Buffer);
	}

	String Environment::ExpandEnvironmentVariables(const String& Path)
	{
		char Buffer[4096]{};
		ExpandEnvironmentStringsA((const char*)Path, Buffer, 4096);

		// In theory, this should be ok, since, most paths will be used for actual file paths, < 260 chars anyways...

		return String(Buffer);
	}

	constexpr bool Environment::Is64BitProcess()
	{
		return (sizeof(uintptr_t) == 8);
	}
}