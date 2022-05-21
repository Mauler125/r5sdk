#include "stdafx.h"
#include "Process.h"
#include "Path.h"
#include "Environment.h"
#include "Directory.h"
#include <winternl.h>
#include <shellapi.h>

//
// Internal routine for getting system information
//

typedef LONG (NTAPI *NtSuspendProcess)(HANDLE);
typedef LONG (NTAPI *NtResumeProcess)(HANDLE);

typedef struct _THREAD_BASIC_INFORMATION
{

	NTSTATUS                ExitStatus;
	PVOID                   TebBaseAddress;
	CLIENT_ID               ClientId;
	KAFFINITY               AffinityMask;
	KPRIORITY               Priority;
	KPRIORITY               BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

#pragma comment(lib, "ntdll.lib")

namespace Diagnostics
{
	Process::Process()
		: _Handle(nullptr), _HasLoaded(false), _Access(PROCESS_ALL_ACCESS), _ProcessInfo()
	{
	}

	Process::Process(ProcessInfo Pi)
		: _Handle(nullptr), _HasLoaded(true), _Access(PROCESS_ALL_ACCESS)
	{
		this->_ProcessInfo = Pi;
	}

	Process::~Process()
	{
		this->Close();
	}

	Process::Process(const Process& Rhs)
		: _Handle(nullptr), _HasLoaded(false), _Access(PROCESS_ALL_ACCESS)
	{
		this->_HasLoaded = Rhs._HasLoaded;
		this->_ProcessInfo = Rhs._ProcessInfo;
	}

	Process::Process(Process&& Rhs)
	{
		this->_Handle = Rhs._Handle;
		this->_HasLoaded = Rhs._HasLoaded;
		this->_Access = Rhs._Access;
		this->_ProcessInfo = Rhs._ProcessInfo;

		Rhs._Handle = nullptr;
		Rhs._Access = PROCESS_ALL_ACCESS;
		Rhs._HasLoaded = false;
		Rhs._ProcessInfo = {};
	}

	void Process::Close()
	{
		if (this->_Handle)
			CloseHandle(this->_Handle);

		this->_Handle = nullptr;
	}

	void Process::CloseMainWindow()
	{
		auto mWindow = this->GetProcessMainWindowHandle();
		if (!mWindow)
			return;

		PostMessageA(mWindow, WM_CLOSE, NULL, NULL);
	}

	bool Process::IsAlive()
	{
		if (!this->AquireProcessHandle())
			return false;

		return (WaitForSingleObject(this->_Handle, 0) == WAIT_TIMEOUT);
	}

	void Process::Kill()
	{
		if (!this->AquireProcessHandle())
			return;

		TerminateProcess(this->_Handle, -1);
	}

	void Process::Suspend()
	{
		if (!this->AquireProcessHandle())
			return;

		auto pSuspend = (NtSuspendProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSuspendProcess");

		pSuspend(this->_Handle);
	}

	void Process::Resume()
	{
		if (!this->AquireProcessHandle())
			return;

		auto pResume = (NtResumeProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtResumeProcess");

		pResume(this->_Handle);
	}

	void Process::WaitForExit()
	{
		if (!this->AquireProcessHandle())
			return;

		WaitForSingleObject(this->_Handle, INFINITE);
	}

	void Process::WaitForInputIdle()
	{
		if (!this->AquireProcessHandle())
			return;

		::WaitForInputIdle(this->_Handle, INFINITE);
	}

	bool Process::InjectModule(const string& ModulePath)
	{
		if (!this->AquireProcessHandle())
			return false;

		BOOL Is32BitProcess = false;
		if (!IsWow64Process(this->_Handle, &Is32BitProcess))
			return false;

		auto PathBuffer = VirtualAllocEx(this->_Handle, NULL, (SIZE_T)(ModulePath.Length() + 1), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (PathBuffer == NULL)
			return false;

		auto WriteResult = WriteProcessMemory(this->_Handle, PathBuffer, (const char*)ModulePath, (SIZE_T)(ModulePath.Length() + 1), NULL);
		if (WriteResult == FALSE)
			return false;

		auto LoadLibraryProc = this->ResolveInjectionAddress(Is32BitProcess);
		if (LoadLibraryProc == nullptr)
			return false;

		DWORD ThreadID = 0;
		auto ThreadHandle = CreateRemoteThread(this->_Handle, NULL, 0, LoadLibraryProc, PathBuffer, 0, &ThreadID);
		if (ThreadHandle == NULL)
			return false;

		WaitForSingleObject(ThreadHandle, INFINITE);
		return true;
	}

	const string& Process::GetProcessName() const
	{
		return this->_ProcessInfo.ProcessName;
	}

	const uint32_t Process::GetBasePriority() const
	{
		return this->_ProcessInfo.BasePriority;
	}

	const uint32_t Process::GetId() const
	{
		return this->_ProcessInfo.ProcessId;
	}

	const uint32_t Process::GetHandleCount() const
	{
		return this->_ProcessInfo.HandleCount;
	}

	const uint64_t Process::GetPagedSystemMemorySize() const
	{
		return this->_ProcessInfo.PoolPagedBytes;
	}

	const uint64_t Process::GetNonPagedSystemMemorySize() const
	{
		return this->_ProcessInfo.PoolNonPagedBytes;
	}

	const uint64_t Process::GetVirtualMemorySize() const
	{
		return this->_ProcessInfo.VirtualBytes;
	}

	const uint64_t Process::GetPeakVirtualMemorySize() const
	{
		return this->_ProcessInfo.VirtualBytesPeak;
	}

	const uint64_t Process::GetWorkingSet() const
	{
		return this->_ProcessInfo.WorkingSet;
	}

	const uint64_t Process::GetPeakWorkingSet() const
	{
		return this->_ProcessInfo.WorkingSetPeak;
	}

	const uint64_t Process::GetPagedMemorySize() const
	{
		return this->_ProcessInfo.PageFileBytes;
	}

	const uint64_t Process::GetPeakPagedMemorySize() const
	{
		return this->_ProcessInfo.PageFileBytesPeak;
	}

	const uint64_t Process::GetPrivateMemorySize() const
	{
		return this->_ProcessInfo.PrivateBytes;
	}

	const uint32_t Process::GetSessionId() const
	{
		return this->_ProcessInfo.SessionId;
	}

	const HANDLE Process::GetHandle()
	{
		if (!this->AquireProcessHandle())
			return nullptr;

		return this->_Handle;
	}

	const HWND Process::GetMainWindowHandle()
	{
		return this->GetProcessMainWindowHandle();
	}

	const string Process::GetMainWindowTitle()
	{
		auto mHandle = this->GetProcessMainWindowHandle();
		if (!mHandle)
			return "";

		char Buffer[MAX_PATH + 1]{};
		auto mResult = GetWindowTextA(mHandle, Buffer, MAX_PATH);

		return string(Buffer, mResult);
	}

	const uint32_t Process::GetExitCode()
	{
		if (!this->AquireProcessHandle())
			return -1;

		DWORD eCode = -1;
		GetExitCodeProcess(this->_Handle, &eCode);

		return eCode;
	}

	const List<ProcessModule> Process::GetProcessModules()
	{
		auto Result = List<ProcessModule>();

		if (!this->AquireProcessHandle())
			return Result;

		HMODULE ModHandles[1024];
		DWORD ResultRead;

		if (!EnumProcessModulesEx(this->_Handle, ModHandles, sizeof(ModHandles), &ResultRead, LIST_MODULES_ALL))
			return Result;

		for (uint32_t i = 0; i < (ResultRead / sizeof(HMODULE)); i++)
		{
			auto ModuleInfo = ProcessModule();

			MODULEINFO ModInfo{};
			GetModuleInformation(this->_Handle, ModHandles[i], &ModInfo, sizeof(ModInfo));

			ModuleInfo.ModuleMemorySize = ModInfo.SizeOfImage;
			ModuleInfo.EntryPointAddress = (uint64_t)ModInfo.EntryPoint;
			ModuleInfo.BaseAddress = (uint64_t)ModInfo.lpBaseOfDll;

			char Buffer[1024]{};
			GetModuleBaseNameA(this->_Handle, ModHandles[i], Buffer, 1024);
			ModuleInfo.ModuleName = string(Buffer);

			std::memset(Buffer, 0, 1024);
			GetModuleFileNameExA(this->_Handle, ModHandles[i], Buffer, 1024);
			ModuleInfo.FileName = string(Buffer);

			Result.EmplaceBack(std::move(ModuleInfo));
		}

		return Result;
	}

	const List<ProcessThread> Process::GetProcessThreads()
	{
		auto Result = List<ProcessThread>();

		if (!this->AquireProcessHandle())
			return Result;

		auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
			return Result;

		THREADENTRY32 TInfo{};
		TInfo.dwSize = sizeof(THREADENTRY32);

		if (!Thread32First(hSnapshot, &TInfo))
		{
			CloseHandle(hSnapshot);
			return Result;
		}

		do
		{
			if (TInfo.th32OwnerProcessID == this->_ProcessInfo.ProcessId)
			{
				auto ThreadInfo = ProcessThread();

				ThreadInfo.Id = TInfo.th32ThreadID;
				ThreadInfo.BasePriority = TInfo.tpBasePri;

				auto hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, TInfo.th32ThreadID);
				if (hThread == NULL)
					continue;

				THREAD_BASIC_INFORMATION ExThreadInfo{};
				uint64_t ExThreadStart = 0;
				NtQueryInformationThread(hThread, (THREADINFOCLASS)0x0, &ExThreadInfo, sizeof(ExThreadInfo), NULL);
				NtQueryInformationThread(hThread, (THREADINFOCLASS)0x9, &ExThreadStart, sizeof(uint64_t), NULL);

				ThreadInfo.CurrentPriority = ExThreadInfo.Priority;
				ThreadInfo.StartAddress = ExThreadStart;

				CloseHandle(hThread);
				Result.EmplaceBack(std::move(ThreadInfo));
			}

		} while (Thread32Next(hSnapshot, &TInfo));

		CloseHandle(hSnapshot);
		return Result;
	}

	Process Process::GetProcessById(uint32_t Pid)
	{
		uint32_t OurId[1] = { Pid };
		auto ProcessInfo = Process::GetProcessInfos({ OurId });
		if (ProcessInfo.Count() == 0)
			return Process();

		return Process(ProcessInfo[0]);
	}

	List<Process> Process::GetProcessesByName(const string& Name)
	{
		auto ProcessInfos = Process::GetProcessInfos({});
		auto Result = List<Process>();

		auto NameCompare = Name.ToLower();

		// Iterate and filter
		for (uint32_t i = 0; i < ProcessInfos.Count(); i++)
		{
			if (ProcessInfos[i].ProcessName.ToLower() == NameCompare)
				Result.EmplaceBack(ProcessInfos[i]);
		}

		return Result;
	}

	List<Process> Process::GetProcesses()
	{
		auto ProcessInfos = Process::GetProcessInfos({});
		auto Result = List<Process>(ProcessInfos.Count());
		
		// Iterate and setup
		for (uint32_t i = 0; i < ProcessInfos.Count(); i++)
			Result.EmplaceBack(ProcessInfos[i]);

		return Result;
	}

	Process Process::GetCurrentProcess()
	{
		uint32_t OurId[1] = { ::GetCurrentProcessId() };

		return Process(Process::GetProcessInfos({ OurId })[0]);
	}

	Process Process::Start(const string& FileName)
	{
		return Process::Start(ProcessStartInfo(FileName));
	}

	Process Process::Start(const string& FileName, const string& Arguments)
	{
		return Process::Start(ProcessStartInfo(FileName, Arguments));
	}

	Process Process::Start(const ProcessStartInfo& Start)
	{
		// Create it depending on launch type
		if (Start.UseShellExecute)
		{
			SHELLEXECUTEINFOA StartInfo{};
			StartInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			StartInfo.cbSize = sizeof(StartInfo);

			switch (Start.WindowStyle)
			{
			case ProcessWindowStyle::Hidden:
				StartInfo.nShow = SW_HIDE;
				break;
			case ProcessWindowStyle::Minimized:
				StartInfo.nShow = SW_SHOWMINIMIZED;
				break;
			case ProcessWindowStyle::Maximized:
				StartInfo.nShow = SW_SHOWMAXIMIZED;
				break;
			default:
				StartInfo.nShow = SW_SHOWNORMAL;
				break;
			}

			if (Start.FileName.Length() != 0)
				StartInfo.lpFile = (char*)Start.FileName;
			if (Start.Verb.Length() != 0)
				StartInfo.lpVerb = (char*)Start.Verb;
			if (Start.Arguments.Length() != 0)
				StartInfo.lpParameters = (char*)Start.Arguments;
			if (Start.WorkingDirectory.Length() != 0)
				StartInfo.lpDirectory = (char*)Start.WorkingDirectory;

			StartInfo.fMask |= SEE_MASK_FLAG_DDEWAIT;

			if (!ShellExecuteExA(&StartInfo))
				return Process();

			return GetProcessById(GetProcessId(StartInfo.hProcess));
		}
		else
		{
			STARTUPINFOA StartInfo{};
			PROCESS_INFORMATION ProcessInfo{};

			string CommandLine;
			if (Start.FileName.StartsWith("\"") && Start.FileName.EndsWith("\""))
				CommandLine += Start.FileName;
			else
				CommandLine = "\"" + Start.FileName + "\"";

			if (!string::IsNullOrWhiteSpace(Start.Arguments))
			{
				CommandLine += " " + Start.Arguments;
			}

			DWORD CreationFlags = 0;
			if (Start.CreateNoWindow)
				CreationFlags |= CREATE_NO_WINDOW;

			auto WorkingDirectory = (Start.WorkingDirectory.Length() == 0) ? IO::Directory::GetCurrentDirectory() : Start.WorkingDirectory;

			if (!CreateProcessA(NULL, (char*)CommandLine, NULL, NULL, TRUE, CreationFlags, NULL, (const char*)WorkingDirectory, &StartInfo, &ProcessInfo))
				return Process();

			return GetProcessById(ProcessInfo.dwProcessId);
		}
	}

	void Process::EnterDebugMode()
	{
		Process::SetPrivilage("SeDebugPrivilege", true);
	}

	void Process::LeaveDebugMode()
	{
		Process::SetPrivilage("SeDebugPrivilege", false);
	}

	Process& Process::operator=(const Process& Rhs)
	{
		this->_Access = Rhs._Access;
		this->_HasLoaded = Rhs._HasLoaded;
		this->_ProcessInfo = Rhs._ProcessInfo;
		this->_Handle = nullptr;

		return *this;
	}

	bool Process::operator==(const nullptr_t Rhs) const
	{
		if (this->_Handle == nullptr && this->_HasLoaded == false && this->_ProcessInfo.ProcessId == 0)
			return true;

		return false;
	}

	bool Process::operator!=(const nullptr_t Rhs) const
	{
		return !(*this == Rhs);
	}

	void Process::SetPrivilage(const string& PrivilegeName, bool Enabled)
	{
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		{
			if (GetLastError() == ERROR_NO_TOKEN)
			{
				if (!ImpersonateSelf(SECURITY_IMPERSONATION_LEVEL::SecurityImpersonation))
					return;
				if (!OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
					return;
			}
			else
			{
				return;
			}
		}

		TOKEN_PRIVILEGES tp;
		LUID luid;
		TOKEN_PRIVILEGES tpPrevious;
		DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

		if (!LookupPrivilegeValueA(NULL, (const char*)PrivilegeName, &luid)) 
			return;

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = 0;

		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious);

		if (GetLastError() != ERROR_SUCCESS) 
			return;

		tpPrevious.PrivilegeCount = 1;
		tpPrevious.Privileges[0].Luid = luid;

		if (Enabled)
			tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
		else
			tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

		AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL);
	}

	List<ProcessInfo> Process::GetProcessInfos(const List<uint32_t>& ProcessIdMatch)
	{
		auto Result = List<ProcessInfo>(68);

		ULONG Required = 0; NTSTATUS Status = 0;
		std::unique_ptr<uint8_t[]> ProcessInfoBuffer = nullptr;

		do
		{
			Status = NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS::SystemProcessInformation, (PVOID)ProcessInfoBuffer.get(), Required, &Required);

			if (Status == Process::StatusInfoLengthMismatch)
				ProcessInfoBuffer.reset(new uint8_t[Required]);

		} while (Status == Process::StatusInfoLengthMismatch);

		uint64_t CurrentOffset = 0;

		while (true)
		{
			auto NProcessInfo = (SYSTEM_PROCESS_INFORMATION*)(ProcessInfoBuffer.get() + CurrentOffset);
			auto NProcessId = (uint32_t)(uint64_t)NProcessInfo->UniqueProcessId;

			ProcessInfo Pi;

			Pi.ProcessId = NProcessId;
			Pi.HandleCount = (uint32_t)NProcessInfo->HandleCount;
			Pi.SessionId = (uint32_t)NProcessInfo->SessionId;
			Pi.PoolPagedBytes = (uint64_t)NProcessInfo->QuotaPagedPoolUsage;;
			Pi.PoolNonPagedBytes = (uint64_t)NProcessInfo->QuotaNonPagedPoolUsage;
			Pi.VirtualBytes = (uint64_t)NProcessInfo->VirtualSize;
			Pi.VirtualBytesPeak = (uint64_t)NProcessInfo->PeakVirtualSize;
			Pi.WorkingSetPeak = (uint64_t)NProcessInfo->PeakWorkingSetSize;
			Pi.WorkingSet = (uint64_t)NProcessInfo->WorkingSetSize;
			Pi.PageFileBytesPeak = (uint64_t)NProcessInfo->PeakPagefileUsage;
			Pi.PageFileBytes = (uint64_t)NProcessInfo->PagefileUsage;
			Pi.PrivateBytes = (uint64_t)NProcessInfo->PrivatePageCount;
			Pi.BasePriority = (uint32_t)NProcessInfo->BasePriority;

			if (NProcessInfo->ImageName.Buffer == NULL)
			{
				if (NProcessId == 0x4)
				{
					Pi.ProcessName = "System";
				}
				else if (NProcessId == 0x0)
				{
					Pi.ProcessName = "Idle";
				}
				else
				{
					Pi.ProcessName = string::Format("Process_%d", NProcessId);
				}
			}
			else
			{
				Pi.ProcessName = Process::GetProcessShortName(wstring(NProcessInfo->ImageName.Buffer, NProcessInfo->ImageName.Length / sizeof(WCHAR)).ToString());
			}

			if (!ProcessIdMatch.Empty() && ProcessIdMatch.Contains(NProcessId))
				Result.EmplaceBack(std::move(Pi));
			else if (ProcessIdMatch.Empty())
				Result.EmplaceBack(std::move(Pi));

			if (NProcessInfo->NextEntryOffset == 0)
				break;

			CurrentOffset += NProcessInfo->NextEntryOffset;
		}

		return Result;
	}

	string Process::GetProcessShortName(const string& Name)
	{
		int32_t Slash = -1, Period = -1;

		for (uint32_t i = 0; i < Name.Length(); i++)
		{
			if (Name[i] == '\\')
				Slash = i;
			else if (Name[i] == '.')
				Period = i;
		}

		if (Period == -1)
			Period = Name.Length() - 1;
		else
		{
			auto Ext = Name.Substring(Period);
			if (Ext.ToLower() == ".exe")
				Period--;
			else
				Period = Name.Length() - 1;
		}

		if (Slash == -1)
			Slash = 0;
		else
			Slash++;

		return Name.Substring(Slash, Period - Slash + 1);
	}

	LPTHREAD_START_ROUTINE Process::ResolveInjectionAddress(BOOL Is32BitProcess)
	{
		if (!Is32BitProcess && sizeof(uintptr_t) == 8)
			return (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
		else if (Is32BitProcess && sizeof(uintptr_t) == 4)
			return (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

		//
		// Get the base address of kernel32.dll
		//

		HMODULE ModHandles[1024];
		DWORD ResultRead;

		if (!EnumProcessModulesEx(this->_Handle, ModHandles, sizeof(ModHandles), &ResultRead, LIST_MODULES_32BIT))
			return nullptr;

		HMODULE Kernel32Wow = nullptr;
		for (uint32_t i = 0; i < (ResultRead / sizeof(HMODULE)); i++)
		{
			char ModNameTemp[MAX_PATH];
			if (GetModuleFileNameExA(this->_Handle, ModHandles[i], ModNameTemp, sizeof(ModNameTemp)))
			{
				auto NameLen = strlen(ModNameTemp);
				auto K32Len = strlen("kernel32.dll");

				if (NameLen >= K32Len && _stricmp(ModNameTemp + (NameLen - K32Len), "kernel32.dll") == 0)
				{
					Kernel32Wow = ModHandles[i];
					break;
				}
			}
		}

		if (Kernel32Wow == nullptr)
			return nullptr;

		//
		// We have the base address, now, parse the module for the export
		//

		auto ModulePath = IO::Path::Combine(System::Environment::GetFolderPath(System::SpecialFolder::SystemX86), "kernel32.dll");

		HANDLE ExeHandle = CreateFileA((const char*)ModulePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (ExeHandle == INVALID_HANDLE_VALUE)
			return nullptr;

		HANDLE ExeMapping = CreateFileMappingA(ExeHandle, NULL, PAGE_READONLY, 0, 0, NULL);
		if (ExeMapping == NULL)
		{
			CloseHandle(ExeHandle);
			return nullptr;
		}

		LPVOID ExeBuffer = MapViewOfFile(ExeMapping, FILE_MAP_READ, 0, 0, 0);
		if (ExeBuffer == NULL)
		{
			CloseHandle(ExeMapping);
			CloseHandle(ExeHandle);
			return nullptr;
		}

		PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)ExeBuffer;
		PIMAGE_NT_HEADERS32 NTHeader = (PIMAGE_NT_HEADERS32)((uintptr_t)ExeBuffer + (uintptr_t)DosHeader->e_lfanew);
		PIMAGE_SECTION_HEADER FirstSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)&NTHeader->OptionalHeader + NTHeader->FileHeader.SizeOfOptionalHeader);
		PIMAGE_EXPORT_DIRECTORY ExportDirectory = nullptr;
		PULONG Functions = nullptr, Names = nullptr;
		PUSHORT Ordinals = nullptr;

		uint32_t SectionIndex = 0;

		for (SectionIndex = 0; SectionIndex < NTHeader->FileHeader.NumberOfSections; SectionIndex++)
		{
			if (FirstSectionHeader[SectionIndex].VirtualAddress <= NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
				&& NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress < (FirstSectionHeader[SectionIndex].VirtualAddress + FirstSectionHeader[SectionIndex].Misc.VirtualSize))
			{
				ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)ExeBuffer + FirstSectionHeader[SectionIndex].PointerToRawData +
					NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress - FirstSectionHeader[SectionIndex].VirtualAddress);

				// Resolve each segment (Fn, Name, Ordinal) from the directory
				Functions = (PULONG)((PBYTE)ExeBuffer + FirstSectionHeader[SectionIndex].PointerToRawData +
					ExportDirectory->AddressOfFunctions - FirstSectionHeader[SectionIndex].VirtualAddress);
				Names = (PULONG)((PBYTE)ExeBuffer + FirstSectionHeader[SectionIndex].PointerToRawData +
					ExportDirectory->AddressOfNames - FirstSectionHeader[SectionIndex].VirtualAddress);
				Ordinals = (PUSHORT)((PBYTE)ExeBuffer + FirstSectionHeader[SectionIndex].PointerToRawData +
					ExportDirectory->AddressOfNameOrdinals - FirstSectionHeader[SectionIndex].VirtualAddress);

				break;
			}
		}

		if (ExportDirectory == nullptr)
		{
			UnmapViewOfFile(ExeBuffer);
			CloseHandle(ExeMapping);
			CloseHandle(ExeHandle);

			return nullptr;
		}

		LPTHREAD_START_ROUTINE ResultAddress = nullptr;

		for (uint32_t ordinal = 0; (ordinal < ExportDirectory->NumberOfFunctions) && (ResultAddress == nullptr); ordinal++)
		{
			for (uint32_t i = 0; i < ExportDirectory->NumberOfNames; i++)
			{
				if (ordinal == Ordinals[i])
				{
					auto FunctionName = (char*)((PBYTE)ExeBuffer + FirstSectionHeader[SectionIndex].PointerToRawData +
						Names[i] - FirstSectionHeader[SectionIndex].VirtualAddress);

					if (_strnicmp(FunctionName, "LoadLibraryA", strlen("LoadLibraryA")) == 0)
					{
						ResultAddress = (LPTHREAD_START_ROUTINE)((uintptr_t)Kernel32Wow + Functions[Ordinals[i]]);
						break;
					}
				}
			}
		}

		UnmapViewOfFile(ExeBuffer);
		CloseHandle(ExeMapping);
		CloseHandle(ExeHandle);

		return ResultAddress;
	}

	HWND Process::GetProcessMainWindowHandle()
	{
		if (!this->_HasLoaded)
			return nullptr;

		struct params
		{
			HWND hWnd;
			DWORD pId;
		} Params;

		Params.hWnd = nullptr;
		Params.pId = this->_ProcessInfo.ProcessId;

		auto eResult = EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL
		{
			auto pParams = (params*)(lParam);

			DWORD ProcessId;
			if (GetWindowThreadProcessId(hWnd, &ProcessId) && ProcessId == pParams->pId)
			{
				pParams->hWnd = hWnd;
				return FALSE;
			}

			return TRUE;

		}, (LPARAM)&Params);

		return Params.hWnd;
	}

	bool Process::AquireProcessHandle()
	{
		if (!this->_HasLoaded)
			return false;
		if (this->_Handle != nullptr)
			return true;

		// Default to all access if we can get it, required for injection
		this->_Access = PROCESS_ALL_ACCESS;

		auto Handle = OpenProcess(this->_Access, FALSE, this->_ProcessInfo.ProcessId);
		if (Handle == NULL)
		{
			this->_Access = (PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_VM_READ | PROCESS_TERMINATE);
			Handle = OpenProcess(this->_Access, FALSE, this->_ProcessInfo.ProcessId);
		}

		// TODO: insert error handling / exceptions when the process no longer exists...

		if (Handle == NULL)
			return false;

		this->_Handle = Handle;
		return true;
	}
}