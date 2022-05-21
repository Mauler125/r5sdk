#pragma once

#include <memory>
#include <Windows.h>
#include "Stream.h"
#include "StringBase.h"

namespace IO
{
	// ProcessStream supports reading and writing from processes
	class ProcessStream : public Stream
	{
	public:
		ProcessStream();
		ProcessStream(uint32_t PID);
		ProcessStream(const string& ProcessName);
		ProcessStream(HANDLE ProcessHandle);
		ProcessStream(HANDLE ProcessHandle, bool LeaveOpen);
		virtual ~ProcessStream();

		// Implement Getters and Setters
		virtual bool CanRead();
		virtual bool CanWrite();
		virtual bool CanSeek();
		virtual bool GetIsEndOfFile();
		virtual uint64_t GetLength();
		virtual uint64_t GetPosition();
		virtual void SetLength(uint64_t Length);
		virtual void SetPosition(uint64_t Position);

		// Implement functions
		virtual void Close();
		virtual void Flush();
		virtual void Seek(uint64_t Offset, SeekOrigin Origin);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual uint64_t Read(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count);
		virtual void Write(uint8_t* Buffer, uint64_t Offset, uint64_t Count, uint64_t Position);

		// Retreive the internal process handle
		virtual HANDLE GetProcessHandle() const;

	private:
		// Process flags cached
		bool _KeepOpen;
		HANDLE _ProcessHandle;
		uint64_t _Position;

		// Sets up the ProcessStream
		void SetupStream(HANDLE ProcessHandle, bool LeaveOpen);
	};
}