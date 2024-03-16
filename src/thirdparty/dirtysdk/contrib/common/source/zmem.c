/*H********************************************************************************/
/*!
    \File zmem.c

    \Description
        Memory implementations for use on all platforms.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/16/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "zmemtrack.h"
#include "zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZMemAlloc

    \Description
        Allocate some memory

    \Input  uSize  - amount of memory, in bytes, to allocate
    
    \Output void * - pointer to an allocated memory chunk

    \Version 03/16/2005 (jfrank)
*/
/********************************************************************************F*/
void *ZMemAlloc(uint32_t uSize)
{
    void *pMem;
    if ((pMem = (void *)malloc(uSize)) != NULL)
    {
        ds_memset(pMem, 0xCD, uSize);
        ZMemtrackAlloc(pMem, uSize, 0);
    }
    return(pMem);
}


/*F********************************************************************************/
/*!
    \Function ZMemFree

    \Description
        Free a previously allocated chunk of memory

    \Input  void *pMem - pointer to an allocated memory chunk
    
    \Output None

    \Version 03/16/2005 (jfrank)
*/
/********************************************************************************F*/
uint32_t ZMemFree(void *pMem)
{
    uint32_t uSize;
    ZMemtrackFree(pMem, &uSize);
    ds_memset(pMem, 0xEF, uSize);
    free(pMem);
    return(uSize);
}
