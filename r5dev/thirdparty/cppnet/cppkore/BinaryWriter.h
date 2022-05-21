#pragma once

#include <memory>
#include "Stream.h"
#include "StringBase.h"

namespace IO
{
	// BinaryWriter supports writing to binary data streams
	class BinaryWriter
	{
	public:
		BinaryWriter();
		BinaryWriter(std::unique_ptr<Stream> Stream);
		BinaryWriter(std::unique_ptr<Stream> Stream, bool LeaveOpen);
		BinaryWriter(Stream* Stream);
		BinaryWriter(Stream* Stream, bool LeaveOpen);
		~BinaryWriter();

		template<class T>
		// Writes data of type T to the stream
		void Write(T Value)
		{
			if (!this->BaseStream)
				IOError::StreamBaseStream();

			this->BaseStream->Write((uint8_t*)&Value, 0, sizeof(T));
		}

		// Writes data to the stream
		void Write(std::unique_ptr<uint8_t[]>& Buffer, uint64_t Count);
		// Writes data to the stream to the specified buffer
		void Write(uint8_t* Buffer, uint64_t Index, uint64_t Count);
		// Writes data to the stream to the specified buffer
		void Write(void* Buffer, uint64_t Index, uint64_t Count);

		// Writes a null-terminated string to the stream
		void WriteCString(const string& Value);
		// Writes a wide null-terminated string to the stream
		void WriteWCString(const wstring& Value);
		// Writes a already predetermined size string to the stream
		void WriteSizeString(const string& Value);
		// Writes a .NET string to the stream
		void WriteNetString(const string& Value);
		
		// Writes an integer encoded into 7 bits, top bit = read more
		void WriteVarInt(uint32_t Value);

		// Writes padding bytes (0x0) to the stream
		void Pad(uint64_t Count);

		// Get the underlying stream
		Stream* GetBaseStream() const;
		// Close the BinaryWriter and underlying stream
		void Close();

	private:
		std::unique_ptr<Stream> BaseStream;
		bool _LeaveOpen;
	};
}