/*H********************************************************************************/
/*!
    \File cryptrand.c

    \Description
        Cryptographic random number generator using system-defined rng and chacha
        cipher.

    \Notes
        Native implementations used on: Android, Apple OSX, Apple iOS, Linux, PS4, Windows
        See the platform specific cryptrand source files for more details.

        References: http://tools.ietf.org/html/rfc4086
                    http://randomnumber.org/links.htm

    \Copyright
        Copyright (c) 2012-2020 Electronic Arts Inc.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "cryptrandpriv.h"

#if defined(CRYPTRAND_LINUX)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(CRYPTRAND_APPLE)
#include <errno.h>
#include <Security/Security.h>
#elif defined(CRYPTRAND_WINDOWS)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <bcrypt.h>
#elif defined(CRYPTRAND_SONY)
#include <sce_random.h>
#endif

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "cryptrandcommon.h"

/*** Type Definitions *************************************************************/

//! internal rand state
typedef struct CryptRandRefT
{
    CryptRandCommonT Common;    //!< common state
    int32_t iMemGroup;          //!< memgroup id
    void *pMemGroupUserData;    //!< user data associated with memgroup

    #if defined(CRYPTRAND_LINUX)
    int32_t iFd;                //!< urandom file descriptor
    #elif defined(CRYPTRAND_WINDOWS)
    BCRYPT_ALG_HANDLE hAlgProvider;
    #endif
} CryptRandRefT;

/*** Variables ********************************************************************/

//! global rand state
static CryptRandRefT *_CryptRand_pState = NULL;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _CryptRandGetEntropy

    \Description
        Get random data from the entropy pool

    \Input *pBuffer - [out] random data
    \Input iBufSize - size of random data

    \Version 01/24/2019 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CryptRandGetEntropy(uint8_t *pBuffer, int32_t iBufSize)
{
    #if defined(CRYPTRAND_LINUX)
    if (read(_CryptRand_pState->iFd, pBuffer, iBufSize) == iBufSize)
    {
        return(0);
    }
    NetPrintf(("cryptrand: read(/dev/urandom) failed (err=%s)\n", DirtyErrGetName(errno)));
    #elif defined(CRYPTRAND_APPLE)
    int32_t iResult;
    if ((iResult = SecRandomCopyBytes(kSecRandomDefault, iBufSize, pBuffer)) >= 0)
    {
        return(0);
    }
    NetPrintf(("cryptrand: SecRandomCopyBytes() failed (err=%s)\n", DirtyErrGetName(iResult)));
    #elif defined(CRYPTRAND_WINDOWS)
    if (BCryptGenRandom(_CryptRand_pState->hAlgProvider, pBuffer, iBufSize, 0) == S_OK)
    {
        return(0);
    }
    NetPrintf(("cryptrand: BCryptGetRandom() failed (err=%d)\n", GetLastError()));
    #elif defined(CRYPTRAND_SONY)
    int32_t iResult;
    if ((iResult = sceRandomGetRandomNumber(pBuffer, iBufSize)) == SCE_OK)
    {
        return(0);
    }
    NetPrintf(("cryptrand: sceRandomGetRandomNumber() failed (err=%s)\n", DirtyErrGetName(iResult)));
    #endif
    return(-1);
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CryptRandInit

    \Description
        Initialize CryptRand module

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptRandInit(void)
{
    CryptRandRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query memgroup info
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate state memory
    if ((pState = (CryptRandRefT *)DirtyMemAlloc(sizeof(*pState), CRYPTRAND_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("cryptrand: failed to allocate state\n"));
        return(-1);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->Common.GetEntropy = &_CryptRandGetEntropy;

    #if defined(CRYPTRAND_LINUX)
    // open urandom
    if ((pState->iFd = open("/dev/urandom", O_RDONLY)) < 0)
    {
        DirtyMemFree(pState, CRYPTRAND_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(-1);
    }
    #elif defined(CRYPTRAND_WINDOWS)
    // open algorithm provider
    if (BCryptOpenAlgorithmProvider(&pState->hAlgProvider, BCRYPT_RNG_ALGORITHM, NULL, 0) != S_OK)
    {
        DirtyMemFree(pState, CRYPTRAND_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(-1);
    }
    #endif
    _CryptRand_pState = pState;

    // init the crypt rand state after the proper setup has been done
    if (CryptRandCommonInit(&pState->Common) < 0)
    {
        CryptRandShutdown();
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptRandShutdown

    \Description
        Shut down the CryptRand module

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void CryptRandShutdown(void)
{
    CryptRandRefT *pState = _CryptRand_pState;

    #if defined(CRYPTRAND_LINUX)
    // close file descriptor
    close(pState->iFd);
    pState->iFd = -1;
    #elif defined(CRYPTRAND_WINDOWS)
    // close algorithm provider
    BCryptCloseAlgorithmProvider(pState->hAlgProvider, 0);
    pState->hAlgProvider = 0;
    #endif

    // free memory and reset state pointer
    DirtyMemFree(pState, CRYPTRAND_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    _CryptRand_pState = NULL;
}

/*F********************************************************************************/
/*!
    \Function CryptRandGet

    \Description
        Get random data

    \Input *pBuffer - [out] random data
    \Input iBufSize - size of random data

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void CryptRandGet(uint8_t *pBuffer, int32_t iBufSize)
{
    CryptRandCommonGet(&_CryptRand_pState->Common, pBuffer, iBufSize);
}
