/*H********************************************************************************/
/*!
    \File cryptrandcommon.c

    \Description
        Cryptographic random number generator using system-defined rng and chacha
        cipher.

    \Notes
        Native implementations used on: Android, Apple OSX, Apple iOS, Linux, PS4, Windows
        See the platform specific cryptrand source files for more details.

        References: http://tools.ietf.org/html/rfc4086
                    http://randomnumber.org/links.htm

    \Copyright
        Copyright (c) 2012-2019 Electronic Arts Inc.

    \Version 12/05/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtylib.h"
#include "cryptrandcommon.h"

/*** Defines **********************************************************************/

//! key rotation interval in bytes
#define CRYPTRAND_KEY_INTERVAL (4*1024)

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CryptRandCommonInit

    \Description
        Initialize CryptRand module's common state

    \Input *pCommon     - common state

    \Output
        int32_t         - result of initialization (0=success, negative=failure)

    \Version 01/24/2019 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptRandCommonInit(CryptRandCommonT *pCommon)
{
    uint8_t aKey[32];

    // pull key from entropy source
    if (pCommon->GetEntropy(aKey, sizeof(aKey)) != 0)
    {
        NetPrintf(("cryptrand: unable to pull bytes from entropy for cipher key\n"));
        return(-1);
    }

    // initialize the stream cipher and initialize bytes left
    CryptChaChaInit(&pCommon->Cipher, aKey, sizeof(aKey));
    pCommon->iBytesLeft = CRYPTRAND_KEY_INTERVAL;

    // clear key from stack memory
    ds_memclr(aKey, sizeof(aKey));

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptRandCommonGet

    \Description
        Get random data

    \Input *pCommon     - common state
    \Input *pBuffer     - [out] random bytes
    \Input iBufSize     - size of the random bytes we are requesting

    \Version 01/24/2019 (eesponda)
*/
/********************************************************************************F*/
void CryptRandCommonGet(CryptRandCommonT *pCommon, uint8_t *pBuffer, int32_t iBufSize)
{
    uint8_t aNonce[12];
    uint8_t bGetEntropySuccess = TRUE;

    // check if we need to rotate the key (which pulls from entropy)
    if (pCommon->iBytesLeft == 0)
    {
        bGetEntropySuccess &= (CryptRandCommonInit(pCommon) == 0);
    }

    // get nonce from entropy source
    bGetEntropySuccess &= (pCommon->GetEntropy(aNonce, sizeof(aNonce)) == 0);

    // abort if getting entropy failed
    if (bGetEntropySuccess == FALSE)
    {
        /* in the case that getting bytes from the entropy pool fails, we have no other alternative
           other than to abort. there is no secure fallback we could use and most notable software
           prng will abort in this case.
           in an attempt to prevent this case at all costs we make sure that this operation succeeds
           in the initialize, otherwise the initialization fails which prevents the startup of
           netconn. it is better that we fail then sacrifice the security guarentees of our software. */
        NetPrintf(("cryptrand: unable to pull bytes from entropy pool for encrypt; aborting\n"));
        *(volatile uint8_t*)(0) = 0;
    }

    // encrypt bytes to get our pseudo-random data
    CryptChaChaEncrypt(&pCommon->Cipher, pBuffer, iBufSize, aNonce, sizeof(aNonce), NULL, 0, NULL, 0);
    pCommon->iBytesLeft = (iBufSize >= pCommon->iBytesLeft) ? 0 : pCommon->iBytesLeft - iBufSize;
}
