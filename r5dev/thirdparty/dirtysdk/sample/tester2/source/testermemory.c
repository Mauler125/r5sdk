/*H********************************************************************************/
/*!
    \File testermemory.c

    \Description
        Implement dirtysock memory functions like DirtyMemAlloc and DirtyMemFree.

    \Version 04/01/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"
#include "testermemory.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "libsample/zmem.h"



/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

static uint32_t _TesterMemory_bDebug = FALSE;

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterMemorySetDebug

    \Description
        Enable/disable verbose memory debugging.

    \Input bDebug   - TRUE=enable, FALSE=disable
    
    \Output
        None.

    \Version 11/01/2005 (jbrookes)
*/
/********************************************************************************F*/
void TesterMemorySetDebug(uint32_t bDebug)
{
    _TesterMemory_bDebug = bDebug;
}


/*F********************************************************************************/
/*!
    \Function DirtyMemAlloc

    \Description
        Implementation of the required DirtySock memory allocation routine.

    \Input iSize              - size of memory to allocate
    \Input iMemModule         - memory module id
    \Input iMemGroup          - memory group id
    \Input *pMemGroupUserData - user data associated with mem group
    
    \Output
        void *                - pointer to newly allocated memory, or NULL

    \Version 11/01/2005 (jbrookes)
    \Version 11/19/2008 (mclouatre) adding suppor for mem group user data
*/
/********************************************************************************F*/
#if !defined(DIRTYCODE_DLL)
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void * pMemGroupUserData)
#else
void *DirtyMemAlloc2(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void * pMemGroupUserData)
#endif
{
    void *pMem = ZMemAlloc(iSize);
    if (_TesterMemory_bDebug)
    {
        DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    return(pMem);
}

/*F********************************************************************************/
/*!
    \Function DirtyMemFree

    \Description
        Implementation of the required DirtySock memory free routine.

    \Input *pMem              - pointer to memory block to free
    \Input iMemModule         - memory module id
    \Input iMemGroup          - memory group id
    \Input *pMemGroupUserData - user data associated with mem group
    
    \Output
        None.

    \Version 11/01/2005 (jbrookes)
    \Version 11/19/2008 (mclouatre) adding suppor for mem group user data
*/
/********************************************************************************F*/
#if !defined(DIRTYCODE_DLL)
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#else
void DirtyMemFree2(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#endif
{
    #if DIRTYCODE_DEBUG
    uint32_t uSize = ZMemFree(pMem);
    #else
    ZMemFree(pMem);
    #endif
    
    if (_TesterMemory_bDebug)
    {
        DirtyMemDebugFree(pMem, uSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
}



