/*H********************************************************************************/
/*!
    \File dirtymem.c

    \Description
        DirtySock memory allocation routines.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/12/2005 (jbrookes) First Version
    \Version 11/19/2008 (mclouatre) Adding pMemGroupUserData to mem groups
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

/*** Defines **********************************************************************/

#define DIRTYMEM_MAXGROUPS  (16)

/*** Type Definitions *************************************************************/

typedef struct DirtyMemInfoT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
} DirtyMemInfoT;


/*** Variables ********************************************************************/

static int32_t _DirtyMem_iGroup = 0;
static DirtyMemInfoT _DirtyMem_iGroupStack[DIRTYMEM_MAXGROUPS] = { {'dflt', NULL} };

//#if defined(DIRTYCODE_DLL)

static DirtyMemFreeT  *_DirtyMem_pMemFreeFunc  = NULL;
static DirtyMemAllocT *_DirtyMem_pMemAllocFunc = NULL;

//#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyMemGroupEnter

    \Description
        Set group that will be used for allocations.

    \Input iMemGroup            - group id
    \Input *pMemGroupUserData   - User-provided data pointer

    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/19/2008 (mclouatre) Adding pMemGroupUserData to mem groups
*/
/********************************************************************************F*/
void DirtyMemGroupEnter(int32_t iMemGroup, void *pMemGroupUserData)
{
    // DIRTYMEM_MAXGROUPS - 1: the first slot is occupied by 'dflt'
    if (_DirtyMem_iGroup >= (DIRTYMEM_MAXGROUPS - 1))
    {
        NetPrintf(("dirtymem: group stack overflow\n"));
        return;
    }
    _DirtyMem_iGroup += 1;
    _DirtyMem_iGroupStack[_DirtyMem_iGroup].iMemGroup = iMemGroup;
    _DirtyMem_iGroupStack[_DirtyMem_iGroup].pMemGroupUserData = pMemGroupUserData;
}

/*F********************************************************************************/
/*!
    \Function DirtyMemGroupLeave

    \Description
        Restore previous group that will be used for allocations.

    \Version 10/13/2005 (jbrookes)
*/
/********************************************************************************F*/
void DirtyMemGroupLeave(void)
{
    if (_DirtyMem_iGroup <= 0)
    {
        NetPrintf(("dirtymem: group stack underflow\n"));
        return;
    }
    _DirtyMem_iGroup -= 1;
}

/*F********************************************************************************/
/*!
    \Function DirtyMemGroupQuery

    \Description
        Return current memory group data.

    \Input *pMemGroup           - [OUT param] pointer to variable to be filled with mem group id
    \Input **ppMemGroupUserData - [OUT param] pointer to variable to be filled with pointer to user data

    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) returned values now passed in [OUT] parameters
*/
/********************************************************************************F*/
void DirtyMemGroupQuery(int32_t *pMemGroup, void **ppMemGroupUserData)
{
    if (pMemGroup != NULL)
    {
        *pMemGroup = _DirtyMem_iGroupStack[_DirtyMem_iGroup].iMemGroup;
    }
    if (ppMemGroupUserData != NULL)
    {
        *ppMemGroupUserData = _DirtyMem_iGroupStack[_DirtyMem_iGroup].pMemGroupUserData;
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyMemDebugAlloc

    \Description
        Display memory allocation information to debug output.

    \Input *pMem                - address of memory being freed
    \Input iSize                - size of allocation
    \Input iMemModule           - memory module
    \Input iMemGroup            - memory group
    \Input *pMemGroupUserData   - pointer to user data

    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) adding pMemGroupUserData parameter
*/
/********************************************************************************F*/
#if DIRTYCODE_DEBUG
void DirtyMemDebugAlloc(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    NetPrintf(("dirtymem: [a] %p mod=%C grp=%C udataptr=%p size=%d\n", pMem, iMemModule, iMemGroup, pMemGroupUserData, iSize));
}
#endif

/*F********************************************************************************/
/*!
    \Function DirtyMemDebugFree

    \Description
        Display memory free information to debug output.

    \Input *pMem                - address of memory being freed
    \Input iSize                - size of allocation (if available), or zero
    \Input iMemModule           - memory module
    \Input iMemGroup            - memory group
    \Input *pMemGroupUserData   - pointer to user data

    \Output
        None.

    \Version 10/13/2005 (jbrookes)
    \Version 11/18/2008 (mclouatre) adding pMemGroupUserData parameter
*/
/********************************************************************************F*/
#if DIRTYCODE_DEBUG
void DirtyMemDebugFree(void *pMem, int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    NetPrintf(("dirtymem: [f] %p mod=%C grp=%C udataptr=%p size=%d\n", pMem, iMemModule, iMemGroup, pMemGroupUserData, iSize));
}
#endif

//#if defined(DIRTYCODE_DLL)

/*F********************************************************************************/
/*!
    \Function DirtyMemFuncSet

    \Description
        This is only avaliable in the DLL mode.
        Set Memory Allocate and Free functions that DirtySDK will use.
        If any of the parameters are NULL it will use the default implementation

    \Input *pMemAlloc  - function pointer to Mem Alloc function
    \Input *pMemFree   - function pointer to Mem Free function

    \Output
        None.

    \Version 3/14/2014 (tcho)
*/
/********************************************************************************F*/
void DirtyMemFuncSet(DirtyMemAllocT *pMemAlloc, DirtyMemFreeT *pMemFree)
{
    _DirtyMem_pMemFreeFunc = pMemFree;
    _DirtyMem_pMemAllocFunc = pMemAlloc;
}


/*F********************************************************************************/
/*!
    \Function DirtyMemAlloc

    \Description
        Only Avaliable in DLL mode.
        Implementation of the required DirtySock memory allocation routine.

    \Input iSize              - size of memory to allocate
    \Input iMemModule         - memory module id
    \Input iMemGroup          - memory group id
    \Input *pMemGroupUserData - user data associated with mem group

    \Output
        void *                - pointer to newly allocated memory, or NULL

    \Version 3/14/2014 (tcho)
*/
/********************************************************************************F*/
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void * pMemGroupUserData)
{
    void *pMem = NULL;

    if (_DirtyMem_pMemAllocFunc != NULL)
    {
        pMem = _DirtyMem_pMemAllocFunc(iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    else
    {
        pMem = (void *)malloc(iSize);
    }

    return(pMem);
}

/*F********************************************************************************/
/*!
    \Function DirtyMemFree

    \Description
        Only Avaliable in DLL mode.
        Implementation of the required DirtySock memory free routine.

    \Input *pMem              - pointer to memory block to free
    \Input iMemModule         - memory module id
    \Input iMemGroup          - memory group id
    \Input *pMemGroupUserData - user data associated with mem group

    \Output
        None.

     \Version 3/14/2014 (tcho)
*/
/********************************************************************************F*/
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    if (_DirtyMem_pMemFreeFunc != NULL)
    {
        _DirtyMem_pMemFreeFunc(pMem, iMemModule, iMemGroup, pMemGroupUserData);
    }
    else
    {
        free(pMem);
    }
}

//#endif
