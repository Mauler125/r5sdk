#ifndef RTECH_PAKDECODE_H
#define RTECH_PAKDECODE_H
#include "rtech/ipakfile.h"

extern size_t Pak_InitDecoder(PakDecoder_s* const decoder, const uint8_t* const inputBuf, uint8_t* const outputBuf,
	const uint64_t inputMask, const uint64_t outputMask, const size_t dataSize, const size_t dataOffset,
	const size_t headerSize, const PakDecodeMode_e decodeMode);

extern bool Pak_StreamToBufferDecode(PakDecoder_s* const decoder, const size_t inLen, const size_t outLen, const PakDecodeMode_e decodeMode);
extern bool Pak_BufferToBufferDecode(uint8_t* const inBuf, uint8_t* const outBuf, const size_t pakSize, const PakDecodeMode_e decodeMode);

extern bool Pak_DecodePakFile(const char* const inPakFile, const char* const outPakFile);

#endif // RTECH_PAKDECODE_H
