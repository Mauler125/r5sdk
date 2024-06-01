#ifndef TIER2_CRYPTUTILS_H
#define TIER2_CRYPTUTILS_H

bool Plat_GenerateRandom(unsigned char* pBuffer, const uint32_t nBufLen, const char*& errorMsg);

typedef unsigned char CryptoIV_t[16];
typedef unsigned char CryptoKey_t[16];

struct CryptoContext_s
{
	CryptoContext_s(const int setKeyBits = 128)
		: keyBits(setKeyBits)
	{
		Assert(setKeyBits == 128 || setKeyBits == 192 || setKeyBits == 256);
		memset(ivData, 0, sizeof(ivData));
	}

	CryptoIV_t ivData;
	int keyBits;
};

bool Crypto_GenerateIV(CryptoContext_s& ctx, const unsigned char* const data, const size_t size);
bool Crypto_CTREncrypt(CryptoContext_s& ctx, const unsigned char* const inBuf, unsigned char* const outBuf,
	const unsigned char* const key, const size_t size);
bool Crypto_CTRDecrypt(CryptoContext_s& ctx, const unsigned char* const inBuf, unsigned char* const outBuf,
	const unsigned char* const key, const size_t size);

#endif // TIER2_CRYPTUTILS_H
