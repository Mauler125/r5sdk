#pragma once

#include <memory>
#include "Stream.h"
#include "ListBase.h"
#include "StringBase.h"

namespace IO
{
	// BinaryReader supports reading binary data streams
	class BinaryReader
	{
	public:
		BinaryReader();
		BinaryReader(std::unique_ptr<Stream> Stream);
		BinaryReader(std::unique_ptr<Stream> Stream, bool LeaveOpen);
		BinaryReader(Stream* Stream);
		BinaryReader(Stream* Stream, bool LeaveOpen);
		~BinaryReader();

		template <class T>
		// Reads data of type T from the stream
		T Read() const
		{
			if (!this->BaseStream)
				IOError::StreamBaseStream();

			T ResultValue{};
			this->BaseStream->Read((uint8_t*)&ResultValue, 0, sizeof(T));

			return ResultValue;
		}
		
		// Reads data from the stream
		std::unique_ptr<uint8_t[]> Read(uint64_t Count, uint64_t& Result);
		// Reads data from the stream to the specified buffer
		uint64_t Read(uint8_t* Buffer, uint64_t Index, uint64_t Count);
		// Reads data from the stream to the specified buffer
		uint64_t Read(void* Buffer, uint64_t Index, uint64_t Count);

		// Reads a null-terminated string from the stream
		string ReadCString();
		// Reads a wide null-terminated string from the stream
		wstring ReadWCString();
		// Reads a size-string from the stream
		string ReadSizeString(uint64_t Size);
		// Reads a .NET string from the stream
		string ReadNetString();

		// Reads an integer encoded into 7 bits, top bit = read more
		uint32_t ReadVarInt();

		// Scan the stream for a given signature
		int64_t SignatureScan(const string& Signature);
		// Scan the stream for a given signature
		int64_t SignatureScan(const string& Signature, uint64_t Offset, uint64_t Count);

		// Scan the process for a given signature (All occurences)
		List<int64_t> SignatureScanAll(const string& Signature);
		// Scan the process for a given signature (All occurences)
		List<int64_t> SignatureScanAll(const string& Signature, uint64_t Offset, uint64_t Count);

		// Get the underlying stream
		Stream* GetBaseStream() const;
		// Close the BinaryReader and underlying stream
		void Close();

	private:
		std::unique_ptr<Stream> BaseStream;
		bool _LeaveOpen;
	};
}