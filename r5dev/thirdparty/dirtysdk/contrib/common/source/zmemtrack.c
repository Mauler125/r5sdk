/*H********************************************************************************/
/*!
    \File zmemtrack.c

    \Description
        Routines for tracking memory allocations.

    \Copyright
        Copyright (c) 2005-2017 Electronic Arts Inc.

    \Version 02/15/2005 (jbrookes) First Version, based on jfrank's implementation.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "zlib.h"
#include "zmem.h"
#include "zmemtrack.h"

/*** Defines **********************************************************************/

#define ZMEMTRACK_MEMDUMPBYTES              (64)        //!< number of bytes to print around the leak
#define ZMEMTRACK_MAXALLOCATIONS            (1024*8)    //!< maximum list allocation size

/*** Type Definitions *************************************************************/

typedef struct ZMemtrackElemT
{
    void *pMem;
    uint32_t uMemSize;
    uint32_t uTag;
} ZMemtrackElemT;

typedef struct ZMemtrackRefT
{
    uint32_t uNumAllocations;
    uint32_t uMaxAllocations;
    uint32_t uTotalAllocations;
    uint32_t uTotalMemory;
    uint32_t uMaxMemory;
    uint8_t bOverflow;
    uint8_t bStarted;
    uint8_t _pad[2];

    ZMemtrackLogCbT *pLoggingCb;
    void *pUserData;

    ZMemtrackElemT MemList[ZMEMTRACK_MAXALLOCATIONS];
} ZMemtrackRefT;

/*** Variables ********************************************************************/

static ZMemtrackRefT _ZMemtrack_Ref;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ZMemtrackLogPrintf

    \Description
        Logs the information from the module

    \Input *pFormat - information to log
    \Input ...      - additional parameters

    \Version 09/18/2017 (eesponda)
*/
/********************************************************************************F*/
static void _ZMemtrackLogPrintf(const char *pFormat, ...)
{
    char strText[2048];
    int32_t iOffset = 0;
    va_list Args;
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;

    // format output
    va_start(Args, pFormat);
    iOffset += ds_vsnprintf(strText+iOffset, sizeof(strText)-iOffset, pFormat, Args);
    va_end(Args);

    // forward to callback, or print if not callback installed
    if (pRef->pLoggingCb != NULL)
    {
        pRef->pLoggingCb(strText, pRef->pUserData);
    }
    else
    {
        ZPrintf("zmemtrack: %s", strText);
    }
}

/*F********************************************************************************/
/*!
    \Function _ZMemtrackPrintLeak

    \Description
        Print a memory leak to debug output.

    \Input *pElem   - pointer to allocation that was leaked

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ZMemtrackPrintLeak(ZMemtrackElemT *pElem)
{
    static const char _hex[] = "0123456789ABCDEF";
    uint32_t uBytes;
    char strOutput[128];
    int32_t iOutput = 2;

    _ZMemtrackLogPrintf("allocated: [%d] bytes at [%p] with tag '%c%c%c%c'\n", pElem->uMemSize, pElem->pMem,
        (uint8_t)(pElem->uTag>>24), (uint8_t)(pElem->uTag>>16), (uint8_t)(pElem->uTag>>8), (uint8_t)(pElem->uTag));

    ds_memset(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    for (uBytes = 0; (uBytes < pElem->uMemSize) && (uBytes < ZMEMTRACK_MEMDUMPBYTES); uBytes++, iOutput += 2)
    {
        unsigned char cByte = ((unsigned char *)(pElem->pMem))[uBytes];
        strOutput[iOutput]   = _hex[cByte>>4];
        strOutput[iOutput+1] = _hex[cByte&0xf];
        strOutput[(iOutput/2)+40] = isprint(cByte) ? cByte : '.';
        if (uBytes > 0)
        {
            if (((uBytes+1) % 16) == 0)
            {
                strOutput[(iOutput/2)+40+1] = '\0';
                _ZMemtrackLogPrintf("%s\n", strOutput);
                ds_memset(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 0;
            }
            else if (((uBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if (((uBytes > ZMEMTRACK_MEMDUMPBYTES) && (uBytes % ZMEMTRACK_MEMDUMPBYTES) != 0) || (pElem->uMemSize < 16))
    {
        strOutput[(iOutput/2)+40+1] = '\0';
        _ZMemtrackLogPrintf("%s\n", strOutput);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ZMemtrackStartup

    \Description
        Start up the ZMemtracking module.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackStartup(void)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    ds_memclr(pRef, sizeof(*pRef));
    pRef->bStarted = TRUE;
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackShutdown

    \Description
        Shut down the ZMemtracking module.

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackShutdown(void)
{
    // dump the current status of the entire module
    ZMemtrackPrint(ZMEMTRACK_PRINTFLAG_TRACKING, 0, NULL);
    _ZMemtrack_Ref.bStarted = FALSE;
}

/*F********************************************************************************/
/*!
    \Function ZMemtrackCallback

    \Description
        Set the logging callback

    \Input *pLoggingCb  - logging function pointer
    \Input *pUserData   - additional data to pass along

    \Version 09/18/2017 (eesponda)
*/
/********************************************************************************F*/
void ZMemtrackCallback(ZMemtrackLogCbT *pLoggingCb, void *pUserData)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    pRef->pLoggingCb = pLoggingCb;
    pRef->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ZMemtrackAlloc

    \Description
        Track an allocation.

    \Input *pMem    - pointer to allocated memory block
    \Input uSize    - size of allocated memory block
    \Input uTag     - allocation tag
    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackAlloc(void *pMem, uint32_t uSize, uint32_t uTag)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;

    // now if we got the memory, add to the list
    if ((pMem == NULL) || (pRef->bStarted == FALSE))
    {
        return;
    }

    // find a clear spot
    for (uMemEntry = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        if (pRef->MemList[uMemEntry].pMem == NULL)
        {
            // get the memory location
            pRef->uTotalMemory += uSize;
            pRef->uNumAllocations += 1;
            pRef->uTotalAllocations += 1;

            // update high-water tracking
            if (pRef->uMaxAllocations < pRef->uNumAllocations)
            {
                pRef->uMaxAllocations = pRef->uNumAllocations;
            }
            if (pRef->uMaxMemory < pRef->uTotalMemory)
            {
                pRef->uMaxMemory = pRef->uTotalMemory;
            }

            // store the info
            pRef->MemList[uMemEntry].pMem = pMem;
            pRef->MemList[uMemEntry].uMemSize = uSize;
            pRef->MemList[uMemEntry].uTag = uTag;

            break;
        }
    }

    // check to see if we ran out of room to store this stuff
    if (uMemEntry == ZMEMTRACK_MAXALLOCATIONS)
    {
        pRef->bOverflow = 1;
    }
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackFree

    \Description
        Track a free operation.

    \Input *pMem    - pointer to allocated memory block
    \Input *pSize   - [out] storage for memory block size

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackFree(void *pMem, uint32_t *pSize)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;

    if ((pMem == NULL) || (pRef->bStarted == FALSE))
    {
        *pSize = 0;
        return;
    }

    for (uMemEntry = 0, *pSize = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        if (pRef->MemList[uMemEntry].pMem == pMem)
        {
            pRef->uTotalMemory -= pRef->MemList[uMemEntry].uMemSize;
            pRef->uNumAllocations -= 1;
            *pSize = pRef->MemList[uMemEntry].uMemSize;
            ds_memclr(&pRef->MemList[uMemEntry], sizeof(pRef->MemList[uMemEntry]));
            break;
        }
    }
}


/*F********************************************************************************/
/*!
    \Function ZMemtrackPrint

    \Description
        Print overall memory info.

    \Input uFlags       - ZMemtrack_PRINTFLAG_*
    \Input uTag         - [optional] if non-zero, only display memory leaks stamped with this tag
    \Input *pModuleName - [optional] pointer to module name

    \Version 02/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void ZMemtrackPrint(uint32_t uFlags, uint32_t uTag, const char *pModuleName)
{
    ZMemtrackRefT *pRef = &_ZMemtrack_Ref;
    uint32_t uMemEntry;

    if (uFlags & ZMEMTRACK_PRINTFLAG_TRACKING)
    {
        _ZMemtrackLogPrintf("memory report\n");
        _ZMemtrackLogPrintf("  maximum number of allocations at once: [%u]\n", pRef->uMaxAllocations);
        _ZMemtrackLogPrintf("  current number of allocations        : [%u]\n", pRef->uNumAllocations);
        _ZMemtrackLogPrintf("  total number of allocations ever     : [%u]\n", pRef->uTotalAllocations);
        _ZMemtrackLogPrintf("  maximum memory allocated             : [%u] bytes\n", pRef->uMaxMemory);
        _ZMemtrackLogPrintf("  current memory allocated             : [%u] bytes\n", pRef->uTotalMemory);
        _ZMemtrackLogPrintf("\n");
    }

    if (pRef->bOverflow)
    {
        _ZMemtrackLogPrintf("WARNING: Allocation watcher overflowed!");
    }

    // see if there were any leaks
    for (uMemEntry = 0; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
    {
        ZMemtrackElemT *pElem = &pRef->MemList[uMemEntry];
        if ((pElem->pMem != NULL) && ((uTag == 0) || (pElem->uTag == uTag)))
        {
            break;
        }
    }

    // if there were leaks, display them
    if (uMemEntry != ZMEMTRACK_MAXALLOCATIONS)
    {
        _ZMemtrackLogPrintf("detected %s memory leaks!\n", pModuleName != NULL ? pModuleName : "");
        for ( ; uMemEntry < ZMEMTRACK_MAXALLOCATIONS; uMemEntry++)
        {
            ZMemtrackElemT *pElem = &pRef->MemList[uMemEntry];
            if ((pElem->pMem != NULL) && ((uTag == 0) || (pElem->uTag == uTag)))
            {
                _ZMemtrackPrintLeak(pElem);
            }
        }
    }

}
