#pragma once

#include <memory>
#include <Windows.h>
#include "StringBase.h"
#include "ProcessInfo.h"
#include "ProcessModule.h"
#include "ProcessThread.h"
#include "ProcessStartInfo.h"

namespace Diagnostics
{
	class Process
	{
	public:
		Process();
		Process(ProcessInfo Pi);
		~Process();

		Process(const Process& Rhs);
		Process(Process&& Rhs);

		// Frees any resources associated with this process
		void Close();
		// Closes a process that has a user interface by sending a close message
		// to its main window
		void CloseMainWindow();
		// Returns whether or not the process is valid and running
		bool IsAlive();
		// Stops the associated process immediately
		void Kill();
		// Suspends the process
		void Suspend();
		// Resumes the process
		void Resume();
		// Waits for the process to exit
		void WaitForExit();
		// Waits for the process to enter an idle state
		void WaitForInputIdle();
		// Injects a module into the given process
		bool InjectModule(const string& ModulePath);

		//
		// Getters
		//

		// Returns the name of the process
		const string& GetProcessName() const;
		// Returns the process base priority
		const uint32_t GetBasePriority() const;
		// Returns the process id
		const uint32_t GetId() const;
		// Returns the process handle count
		const uint32_t GetHandleCount() const;
		// Returns the amount of memory that the system has allocated for the process
		// that can be written to the virtual memory paging file
		const uint64_t GetPagedSystemMemorySize() const;
		// Returns the amount of memory that the system has allocated for the process
		// that can not be written to the virtual memory paging file
		const uint64_t GetNonPagedSystemMemorySize() const;
		// Returns the amount of virtual memory that the process
		// has requested
		const uint64_t GetVirtualMemorySize() const;
		// Returns the maximum amount of virtual memory that the process
		// has requested
		const uint64_t GetPeakVirtualMemorySize() const;
		// Returns the amount of physical memory that the process
		// has requested
		const uint64_t GetWorkingSet() const;
		// Returns the maximum amount of physical memory that the process
		// has requested
		const uint64_t GetPeakWorkingSet() const;
		// Returns the amount of memory that the process has allocated
		// that can be written to the virtual memory paging file
		const uint64_t GetPagedMemorySize() const;
		// Returns the maximum amount of memory that the process has allocated
		// that can be written to the virtual memory paging file
		const uint64_t GetPeakPagedMemorySize() const;
		// Returns the number of bytes that the associated process has allocated that cannot
		// be shared with other processes
		const uint64_t GetPrivateMemorySize() const;
		// Returns the session id
		const uint32_t GetSessionId() const;

		// Returns the process handle
		const HANDLE GetHandle();
		// Returns the main window handle
		const HWND GetMainWindowHandle();
		// Returns the main window title
		const string GetMainWindowTitle();
		// Returns the process exit code, if any
		const uint32_t GetExitCode();
		// Returns a list of process modules
		const List<ProcessModule> GetProcessModules();
		// Returns a list of process threads
		const List<ProcessThread> GetProcessThreads();

		// Attempts to get a process by the process id
		static Process GetProcessById(uint32_t Pid);
		// Attempts to get processes with a specific name
		static List<Process> GetProcessesByName(const string& Name);
		// Gets all processes running on the current system
		static List<Process> GetProcesses();
		// Gets the current process
		static Process GetCurrentProcess();

		// Creates a new process with the given information
		static Process Start(const string& FileName);
		// Creates a new process with the given information
		static Process Start(const string& FileName, const string& Arguments);
		// Creates a new process with the given information
		static Process Start(const ProcessStartInfo& Start);

		// Sets the SeDebugPrivilege token to true for this process
		static void EnterDebugMode();
		// Sets the SeDebugPrivilege token to false for this process
		static void LeaveDebugMode();

		// Assignment operator
		Process& operator=(const Process& Rhs);

		// Equality operator, against nullptr
		bool operator==(const nullptr_t Rhs) const;
		// Inequality operator, against nullptr
		bool operator!=(const nullptr_t Rhs) const;

	private:
		HANDLE _Handle;
		DWORD _Access;
		bool _HasLoaded;
		ProcessInfo _ProcessInfo;

		// Internal routine to get a handle to the process
		bool AquireProcessHandle();
		// Internal routine to locate the main window
		HWND GetProcessMainWindowHandle();
		// Internal routine to get the address of LoadLibraryA
		LPTHREAD_START_ROUTINE ResolveInjectionAddress(BOOL Is32BitProcess);

		// Internal routine to set a process token privilege
		static void SetPrivilage(const string& PrivilegeName, bool Enabled);

		// Internal routine to get a list of processes on the current machines info
		static List<ProcessInfo> GetProcessInfos(const List<uint32_t>& ProcessIdMatch);
		// Internal routine to transform the process name
		static string GetProcessShortName(const string& Name);

		// Internal constant that matches STATUS_INFO_LENGTH_MISMATCH from ntstatus.h
		static constexpr uint32_t StatusInfoLengthMismatch = 0xC0000004;
	};
}