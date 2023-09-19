#include "stdafx.h"
#include "WAV.h"

namespace Assets
{
	WAVFormat::WAVFormat()
		: FrameRate(44100), ChannelsCount(1), BitsPerSample(16), DataEncoding(WAVEncoding::PCM16), DataSize(0)
	{
	}

	// The maximum size of a WAV header
	constexpr static uint32_t MaximumHeaderSize = 0x100;

	// Helper routine to build a WAV header, internal
	const static uint32_t BuildWAVHeader(uint8_t* Buffer, const WAVFormat& Format)
	{
		uint32_t Offset = 4;

		// RIFF <uint32_t>
		*(uint32_t*)Buffer = 0x46464952;

		// Calculate format buffer size based on encoding
		uint32_t WAVHeaderSize = 0;
		uint32_t WAVfmtSize = 0;
		switch (Format.DataEncoding)
		{
		case WAVEncoding::PCM16:
		case WAVEncoding::FLOAT32:
			WAVHeaderSize = 36;
			WAVfmtSize = 16;
			break;
			// TODO: Support other encoding formats...
		}

		// BufferSize <uint32_t>
		*(uint32_t*)(Buffer + Offset) = Format.DataSize + WAVHeaderSize;
		Offset += sizeof(uint32_t);

		// 'WAVfmt ' <uint64_t>
		*(uint64_t*)(Buffer + Offset) = 0x20746D6645564157;
		Offset += sizeof(uint64_t);

		// WAVfmtSize <uint32_t>
		*(uint32_t*)(Buffer + Offset) = WAVfmtSize;
		Offset += sizeof(uint32_t);

		// Encoding <uint16_t>
		*(uint16_t*)(Buffer + Offset) = (uint16_t)Format.DataEncoding;
		Offset += sizeof(uint16_t);

		// Channels <uint16_t>
		*(uint16_t*)(Buffer + Offset) = (uint16_t)Format.ChannelsCount;
		Offset += sizeof(uint16_t);

		// FrameRate <uint32_t>
		*(uint32_t*)(Buffer + Offset) = Format.FrameRate;
		Offset += sizeof(uint32_t);

		// AverageBps <uint32_t>
		*(uint32_t*)(Buffer + Offset) = (Format.FrameRate * (Format.ChannelsCount * (Format.BitsPerSample / 8)));
		Offset += sizeof(uint32_t);

		// BlockAlign <uint16_t>
		*(uint16_t*)(Buffer + Offset) = (uint16_t)(Format.ChannelsCount * (Format.BitsPerSample / 8));
		Offset += sizeof(uint16_t);

		// BitsPerSample
		*(uint16_t*)(Buffer + Offset) = (uint16_t)Format.BitsPerSample;
		Offset += sizeof(uint16_t);

		// 'data' <uint32_t>
		*(uint32_t*)(Buffer + Offset) = 0x61746164;
		Offset += sizeof(uint32_t);

		// DataSize <uint32_t>
		*(uint32_t*)(Buffer + Offset) = Format.DataSize;
		Offset += sizeof(uint32_t);

		// TODO: Some formats require extra data here... MSADPCM/IMA

		return Offset;
	}

	void WAV::WriteWAVHeader(const std::unique_ptr<IO::Stream>& Stream, const WAVFormat& Format)
	{
		uint8_t Header[MaximumHeaderSize];
		auto HeaderSize = BuildWAVHeader(Header, Format);

		Stream->Write(Header, 0, HeaderSize);
	}

	void WAV::WriteWAVHeader(IO::Stream* Stream, const WAVFormat& Format)
	{
		uint8_t Header[MaximumHeaderSize];
		auto HeaderSize = BuildWAVHeader(Header, Format);

		Stream->Write(Header, 0, HeaderSize);
	}
	
	void WAV::WriteWAVHeader(uint8_t* Buffer, const WAVFormat& Format, uint32_t& ResultSize)
	{
		uint8_t Header[MaximumHeaderSize];
		auto HeaderSize = BuildWAVHeader(Header, Format);

		std::memcpy(Buffer, Header, HeaderSize);

		ResultSize = HeaderSize;
	}
}
