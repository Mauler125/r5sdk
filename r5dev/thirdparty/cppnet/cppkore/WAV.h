#pragma once

#include <cstdint>
#include <memory>
#include "Stream.h"

namespace Assets
{
	// The audio encoding format for the WAV file.
	enum class WAVEncoding : uint32_t
	{
		PCM16 = 0x1,
		MSADPCM = 0x2,
		FLOAT32 = 0x3,
		IMA = 0x11,
	};

	// Contains information about the WAV audio format.
	struct WAVFormat
	{
		// The frame/sample rate of this format (44100, 48000, etc)
		uint32_t FrameRate;
		// The total number of channels
		uint32_t ChannelsCount;
		// The total number of bits per sample (16 for PCM, 32 for FLOAT32...)
		uint32_t BitsPerSample;

		// The data encoding format
		WAVEncoding DataEncoding;
		// The size of the wav data block
		uint32_t DataSize;

		WAVFormat();
	};

	// A utility class for building WAV assets.
	class WAV
	{
		// Don't initialize this class
		WAV() = delete;
		~WAV() = delete;

	public:
		
		// Serializes a WAV header to the stream.
		static void WriteWAVHeader(const std::unique_ptr<IO::Stream>& Stream, const WAVFormat& Format);
		// Serializes a WAV header to the stream.
		static void WriteWAVHeader(IO::Stream* Stream, const WAVFormat& Format);

		// Serializes a WAV header to the buffer.
		static void WriteWAVHeader(uint8_t* Buffer, const WAVFormat& Format, uint32_t& ResultSize);
	};
}