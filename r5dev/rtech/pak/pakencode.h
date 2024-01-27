#ifndef RTECH_PAKENCODE_H
#define RTECH_PAKENCODE_H

bool Pak_BufferToBufferEncode(const uint8_t* const inBuf, const uint64_t inLen,
	uint8_t* const outBuf, const uint64_t outLen, const int level);

bool Pak_EncodePakFile(const char* const inPakFile, const char* const outPakFile, const int level);

#endif // RTECH_PAKENCODE_H
