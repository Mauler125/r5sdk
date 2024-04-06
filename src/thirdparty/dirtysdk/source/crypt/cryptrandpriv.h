/*H********************************************************************************/
/*!
    \File cryptrandpriv.h

    \Description
        Internal APIs for the CryptRand module

    \Copyright
        Copyright (c) 2020 Electronic Arts Inc.

    \Version 02/04/2020 (eesponda)
*/
/********************************************************************************H*/

#ifndef _cryptrandpriv_h
#define _cryptrandpriv_h

/*!
\Moduledef CryptRandPriv CryptRandPriv
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

// define OS flavors of random
#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
#define CRYPTRAND_LINUX
#elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_APPLEIOS)
#define CRYPTRAND_APPLE
#elif defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#define CRYPTRAND_WINDOWS
#elif defined(DIRTYCODE_PS4) || defined(DIRTYCODE_PS5)
#define CRYPTRAND_SONY
#elif defined(DIRTYCODE_NX)
#define CRYPTRAND_NX
#endif 

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// initialize the module
int32_t CryptRandInit(void);

// destroy the module
void CryptRandShutdown(void);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _cryptrandpriv_h
