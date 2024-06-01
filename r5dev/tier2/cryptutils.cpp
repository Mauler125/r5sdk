//===========================================================================//
//
// Purpose: A set of utilities to perform encryption/decryption
//
//===========================================================================//
#include "mbedtls/aes.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/gcm.h"

#include "tier2/cryptutils.h"

static BCRYPT_ALG_HANDLE s_algorithmProvider;
bool Plat_GenerateRandom(unsigned char* buffer, const uint32_t bufferSize, const char*& errorMsg)
{
	if (!s_algorithmProvider && (BCryptOpenAlgorithmProvider(&s_algorithmProvider, L"RNG", 0, 0) < 0))
	{
		errorMsg = "Failed to open rng algorithm";
		return false;
	}

	if (BCryptGenRandom(s_algorithmProvider, buffer, bufferSize, 0) < 0)
	{
		errorMsg = "Failed to generate random data";
		return false;
	}

	return true;
}

bool Crypto_GenerateIV(CryptoContext_s& ctx, const unsigned char* const data, const size_t size)
{
	mbedtls_entropy_context entropy;
	mbedtls_entropy_init(&entropy);

	mbedtls_ctr_drbg_context drbg;
	mbedtls_ctr_drbg_init(&drbg);

	const int seedRet = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, data, size);
	int randRet = MBEDTLS_ERR_ERROR_GENERIC_ERROR;

	if (seedRet == 0)
	{
		randRet = mbedtls_ctr_drbg_random(&drbg, ctx.ivData, sizeof(ctx.ivData));
	}

	mbedtls_ctr_drbg_free(&drbg);
	mbedtls_entropy_free(&entropy);

	return randRet == 0;
}

static bool Crypto_CTRCrypt(CryptoContext_s& ctx, const unsigned char* const inBuf,
	unsigned char* const outBuf, const unsigned char* const key, const size_t size)
{
	mbedtls_aes_context aes;
	mbedtls_aes_init(&aes);

	int cryptRet = MBEDTLS_ERR_ERROR_GENERIC_ERROR;
	const int setRet = mbedtls_aes_setkey_enc(&aes, key, ctx.keyBits);

	if (setRet == 0)
	{
		unsigned char nonceCounter[16];
		unsigned char streamBlock[16];

		size_t offset = 0;
		memcpy(nonceCounter, ctx.ivData, sizeof(nonceCounter));

		cryptRet = mbedtls_aes_crypt_ctr(&aes, size, &offset, nonceCounter, streamBlock, inBuf, outBuf);
	}

	mbedtls_aes_free(&aes);
	return cryptRet == 0;
}

bool Crypto_CTREncrypt(CryptoContext_s& ctx, const unsigned char* const inBuf, unsigned char* const outBuf,
	const unsigned char* const key, const size_t size)
{
	return Crypto_CTRCrypt(ctx, inBuf, outBuf, key, size);
}

bool Crypto_CTRDecrypt(CryptoContext_s& ctx, const unsigned char* const inBuf, unsigned char* const outBuf,
	const unsigned char* const key, const size_t size)
{
	return Crypto_CTRCrypt(ctx, inBuf, outBuf, key, size);
}
