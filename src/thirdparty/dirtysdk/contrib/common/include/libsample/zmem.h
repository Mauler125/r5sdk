/*H********************************************************************************/
/*!
    \File zmem.h

    \Description
        ZAlloc (malloc) and ZFree (free) implementations for use on all platforms.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/16/2005 (jfrank) First Version
*/
/********************************************************************************H*/

#ifndef _zmem_h
#define _zmem_h

/*** Include files ****************************************************************/

// include this so we can use zmemtrack alongside the zmem libraries
#include "zmemtrack.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate some memory, uSize bytes in length
DIRTYCODE_API void *ZMemAlloc(uint32_t uSize);

// free a previously allocated memory chunk
DIRTYCODE_API uint32_t ZMemFree(void *pMem);

#ifdef __cplusplus
};
#endif

#endif // _zmem_h

