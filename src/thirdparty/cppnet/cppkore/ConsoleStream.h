#pragma once

#include "Stream.h"
#include "FileMode.h"
#include "FileAccess.h"
#include "FileShare.h"
#include <Windows.h>

namespace IO
{
	// ConsoleStream supports reading and writing from stdin/out
	class ConsoleStream : public Stream
	{
	public:
		ConsoleStream(HANDLE StreamHandle, FileAccess Access);
		virtual ~ConsoleStream();

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

		// Retreive the internal stream handle
		virtual HANDLE GetStreamHandle() const;

	private:
		// FileMode flags cached
		bool _CanRead;
		bool _CanWrite;
		bool _IsPipe;

		// Internal routine to wait for input
		static void WaitForAvailableConsoleInput(HANDLE hHandle, bool IsPipe);

		// The handle
		HANDLE _Handle;
	};
}