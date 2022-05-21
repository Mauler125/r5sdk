#pragma once

#include <memory>
#include "ProcessStream.h"
#include "StringBase.h"
#include "Process.h"

namespace IO
{
	// ProcessReader supports reading a processes memory
	class ProcessReader
	{
	public:
		ProcessReader();
		ProcessReader(const Diagnostics::Process& Process);
		ProcessReader(std::unique_ptr<ProcessStream> Stream);
		ProcessReader(std::unique_ptr<ProcessStream> Stream, bool LeaveOpen);
		ProcessReader(ProcessStream* Stream);
		ProcessReader(ProcessStream* Stream, bool LeaveOpen);
		~ProcessReader();

		template <class T>
		// Reads data of type T from the stream
		T Read(uint64_t Address) const
		{
			if (!this->BaseStream)
				IOError::StreamBaseStream();

			T ResultValue{};
			this->BaseStream->Read((uint8_t*)&ResultValue, 0, sizeof(T), Address);

			return ResultValue;
		}

		// Reads data from the stream
		std::unique_ptr<uint8_t[]> Read(uint64_t Count, uint64_t Address, uint64_t& Result);
		// Reads data from the stream to the specified buffer
		uint64_t Read(uint8_t* Buffer, uint64_t Index, uint64_t Count, uint64_t Address);
		// Reads data from the stream to the specified buffer
		uint64_t Read(void* Buffer, uint64_t Index, uint64_t Count, uint64_t Address);

		// Reads a null-terminated string from the stream
		string ReadCString(uint64_t Address);
		// Reads a size-string from the stream
		string ReadSizeString(uint64_t Size, uint64_t Address);

		// Reads an integer encoded into 7 bits, top bit = read more
		uint32_t ReadVarInt(uint64_t Address);

		// Scan the process for a given signature
		int64_t SignatureScan(const string& Signature, bool ScanAllMemory = false);
		// Scan the process for a given signature
		int64_t SignatureScan(const string& Signature, uint64_t Address, uint64_t Count);

		// Scan the process for a given signature (All occurences)
		List<int64_t> SignatureScanAll(const string& Signature, bool ScanAllMemory = false);
		// Scan the process for a given signature (All occurences)
		List<int64_t> SignatureScanAll(const string& Signature, uint64_t Address, uint64_t Count);

		// Whether or not the process is running
		bool IsRunning() const;

		// Gets the adress of the main module
		uint64_t GetBaseAddress() const;
		// Gets the amount of memory in use
		uint64_t GetMemorySize() const;
		// Gets the size of executable code
		uint64_t GetSizeOfCode() const;

		// Get the underlying stream
		ProcessStream* GetBaseStream() const;
		// Close the ProcessReader and underlying stream
		void Close();

	private:
		std::unique_ptr<ProcessStream> BaseStream;
		bool _LeaveOpen;

		// The default scan size for reading buffers
		constexpr static uint64_t DefaultScanBufferSize = 4096;
	};
}