#include "stdafx.h"
#include "ProcessStream.h"

namespace IO
{
	ProcessStream::ProcessStream()
	{
		this->_ProcessHandle = nullptr;
		this->_KeepOpen = false;
	}

	ProcessStream::ProcessStream(uint32_t PID)
	{
		this->SetupStream(OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID), false);
	}

	ProcessStream::ProcessStream(const string& ProcessName)
	{
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);
		CHAR szProcessName[MAX_PATH];

		cProcesses = cbNeeded / sizeof(DWORD);

		for (DWORD i = 0; i < cProcesses; i++)
		{
			if (aProcesses[i] != NULL)
			{
				auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
				if (hProcess != NULL)
				{
					HMODULE hMod;
					if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					{
						GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName));
						CloseHandle(hProcess);

						if (_strnicmp(ProcessName.begin(), szProcessName, ProcessName.Length()) == 0)
						{
							this->SetupStream(OpenProcess(PROCESS_ALL_ACCESS, FALSE, aProcesses[i]), false);
							return;
						}
					}
				}
			}
		}

		// If we got here, we couldn't find a process, so the handle is null
		throw std::exception("The process handle must not be null");
	}

	ProcessStream::ProcessStream(HANDLE ProcessHandle)
		: ProcessStream(ProcessHandle, false)
	{
	}

	ProcessStream::ProcessStream(HANDLE ProcessHandle, bool LeaveOpen)
	{
		this->SetupStream(ProcessHandle, LeaveOpen);
	}
	
	ProcessStream::~ProcessStream()
	{
		this->Close();
	}

	bool ProcessStream::CanRead()
	{
		return (this->_ProcessHandle);
	}

	bool ProcessStream::CanWrite()
	{
		return (this->_ProcessHandle);
	}

	bool ProcessStream::CanSeek()
	{
		return (this->_ProcessHandle);
	}

	bool ProcessStream::GetIsEndOfFile()
	{
		return false;
	}

	uint64_t ProcessStream::GetLength()
	{
		// Always an unknown due to pages, so we allow full read/write at any address
		return UINT64_MAX;
	}

	uint64_t ProcessStream::GetPosition()
	{
		return this->_Position;
	}

	void ProcessStream::SetLength(uint64_t Length)
	{
		IOError::StreamSetLengthSupport();
	}

	void ProcessStream::SetPosition(uint64_t Position)
	{
		this->Seek(Position, SeekOrigin::Begin);
	}

	void ProcessStream::Close()
	{
		if (!this->_KeepOpen && this->_ProcessHandle)
			CloseHandle(this->_ProcessHandle);

		this->_ProcessHandle = nullptr;
		this->_Position = 0;
	}

	void ProcessStream::Flush()
	{
		// This is a non-cached stream
	}

	void ProcessStream::Seek(uint64_t Offset, SeekOrigin Origin)
	{
		// We are a process, so, SeekOrigin::End doesn't technically apply here...
		if (Origin == SeekOrigin::Current)
			this->_Position += Offset;
		else
			this->_Position = Offset;
	}

	uint64_t ProcessStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		return this->Read(Buffer, Offset, Count, this->_Position);
	}

	uint64_t ProcessStream::Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		if (!this->_ProcessHandle)
			IOError::StreamNotOpen();

		SIZE_T Result = 0;
		ReadProcessMemory(this->_ProcessHandle, (LPCVOID)Position, (LPVOID)(Buffer + Offset), (SIZE_T)Count, &Result);
		this->_Position = (uint64_t)(Position + Count);

		return Result;
	}

	void ProcessStream::Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count)
	{
		this->Write(Buffer, Offset, Count, this->_Position);
	}

	void ProcessStream::Write(uint8_t * Buffer, uint64_t Offset, uint64_t Count, uint64_t Position)
	{
		if (!this->_ProcessHandle)
			IOError::StreamNotOpen();

		SIZE_T Result = 0;
		WriteProcessMemory(this->_ProcessHandle, (LPVOID)Position, (LPCVOID)(Buffer + Offset), Count, &Result);
		this->_Position = (uint64_t)(Position + Count);
	}

	HANDLE ProcessStream::GetProcessHandle() const
	{
		return this->_ProcessHandle;
	}

	void ProcessStream::SetupStream(HANDLE ProcessHandle, bool LeaveOpen)
	{
		if (!ProcessHandle)
			throw std::exception("The process handle must not be null");

		this->_ProcessHandle = ProcessHandle;
		this->_KeepOpen = LeaveOpen;
	}
}