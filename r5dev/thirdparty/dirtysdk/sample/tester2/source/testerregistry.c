/*H********************************************************************************/
/*!
    \File testerregistry.h

    \Description
        Maintains a global registry for the tester2 application.

    \Notes
        This module works a little differently than other DirtySock modules.
        Because of the nature of a registry (a global collector of shared
        information) there is no pointer to pass around - Create simply
        allocates memory of a given size and Destroy frees this memory.
        Registry entries are stored in tagfield format.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/08/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "libsample/zmem.h"
#include "libsample/zlib.h"
#include "testerregistry.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TesterRegistryEntryT
{
    uintptr_t uPtr;
    int32_t iNum;

    char strName[256];
    char strBuffer[1024];
} TesterRegistryEntryT;

// registry module structure - private and not intended for external use
typedef struct TesterRegistryT
{
    int32_t iMaxEntries;            //!< maximum number of entries supported
    int32_t iNumEntries;            //!< current number of entries
    TesterRegistryEntryT pReg[1];   //!< the actual registry (variable size array)
} TesterRegistryT;

/*** Variables ********************************************************************/

static TesterRegistryT *_TesterRegistry = NULL;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _TesterRegistryFindEntry

    \Description
        Looks up a registry entry by name

    \Input *pEntryName  - name of entry we are looking for
    
    \Output
        TesterRegistryEntryT *  - NULL or entry if found

    \Version 01/11/2017 (eesponda)
*/
/********************************************************************************F*/
static TesterRegistryEntryT *_TesterRegistryFindEntry(const char *pEntryName)
{
    TesterRegistryEntryT *pEntry = NULL;
    int32_t iIndex;

    for (iIndex = 0; iIndex < _TesterRegistry->iNumEntries; iIndex += 1)
    {
        if (strcmp(pEntryName, _TesterRegistry->pReg[iIndex].strName) == 0)
        {
            pEntry = &_TesterRegistry->pReg[iIndex];
            break;
        }
    }

    return(pEntry);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterRegistryCreate

    \Description
        Create a new registry of a given size.

    \Input iSize - Size of registry, in bytes, to create.  (<0) for default size
    
    \Output None

    \Notes
        Calling this function creates a global (static, private) registry which
        any module can access.  If TesterRegistryCreate is called multiple times,
        the old registry is blown away (if it existed) the memory is freed, and
        a new registry is created.  No memory will be leaked if TesterRegistryCreate
        is called multiple times.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryCreate(int32_t iSize)
{
    int32_t iMemSize;

    // dump the old registry if it exists  
    if(_TesterRegistry != NULL)
        TesterRegistryDestroy();

    if(iSize < 0)
    {
        // use default size
        iSize = TESTERREGISTRY_SIZE_DEFAULT;
    }
    iMemSize = sizeof(*_TesterRegistry) - sizeof(_TesterRegistry->pReg) + (sizeof(_TesterRegistry->pReg) * iSize);

    // now create a new registry
    _TesterRegistry = ZMemAlloc(iMemSize);
    ds_memclr(_TesterRegistry, iMemSize);
    _TesterRegistry->iMaxEntries = iSize;
}

/*F********************************************************************************/
/*!
    \Function TesterRegistrySetPointer

    \Description
        Set a registry entry for a pointer

    \Input  *pEntryName - the registry entry name
    \Input  *pPtr       - the pointer to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Notes
        Reference count is automatically set to 1. To increment reference count
        use TesterRegistryGetPointer. To decrement reference count use
        TesterRegistryDecrementRefCount.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetPointer(const char *pEntryName, const void *pPtr)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if(pEntryName == NULL)
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    /* find the entry if it exists in the list
       otherwise add a new entry to the list
       note: we don't remove entries just to simplify */

    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        pEntry = &_TesterRegistry->pReg[_TesterRegistry->iNumEntries];
        ds_strnzcpy(pEntry->strName, pEntryName, sizeof(pEntry->strName));
        _TesterRegistry->iNumEntries += 1;
    }

    pEntry->uPtr = (uintptr_t)pPtr;
    return(TESTERREGISTRY_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterRegistrySetNumber

    \Description
        Set a registry entry for a number

    \Input  *pEntryName - the registry entry name
    \Input   iNum       - the number to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetNumber(const char *pEntryName, const int32_t iNum)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if(pEntryName == NULL)
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    /* find the entry if it exists in the list
       otherwise add a new entry to the list
       note: we don't remove entries just to simplify */

    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        pEntry = &_TesterRegistry->pReg[_TesterRegistry->iNumEntries];
        ds_strnzcpy(pEntry->strName, pEntryName, sizeof(pEntry->strName));
        _TesterRegistry->iNumEntries += 1;
    }

    pEntry->iNum = iNum;
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistrySetString

    \Description
        Set a registry entry for a pointer

    \Input  *pEntryName - the registry entry name
    \Input  *pStr       - the string to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetString(const char *pEntryName, const char *pStr)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pStr == NULL))
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name or entry value\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    /* find the entry if it exists in the list
       otherwise add a new entry to the list
       note: we don't remove entries just to simplify */

    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        pEntry = &_TesterRegistry->pReg[_TesterRegistry->iNumEntries];
        ds_strnzcpy(pEntry->strName, pEntryName, sizeof(pEntry->strName));
        _TesterRegistry->iNumEntries += 1;
    }

    ds_strnzcpy(pEntry->strBuffer, pStr, sizeof(pEntry->strBuffer));
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryGetPointer

    \Description
        Get a pointer entry from the registry

    \Input  *pEntryName - the registry entry name
    
    \Output
        void *          - requested pointer, or NULL if no pointer is set

    \Notes
        Reference count is incremented when Get is called. To decrement
        reference count use TesterRegsitryDecrementRefCount.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void *TesterRegistryGetPointer(const char *pEntryName)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if (_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(NULL);
    }

    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        // no entry found
        return(NULL);
    }

    return((void *)pEntry->uPtr);
}

/*F********************************************************************************/
/*!
    \Function TesterRegistryGetNumber

    \Description
        Get a numeric entry from the registry

    \Input  *pEntryName - the registry entry name
    \Input  *pNum       - destination integer
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistryGetNumber(const char *pEntryName, int32_t *pNum)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pNum == NULL))
    {
        ZPrintf("WARNING: TesterRegistryGet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        // no entry found
        *pNum = 0;
        return(TESTERREGISTRY_ERROR_NOSUCHENTRY);
    }

    *pNum = pEntry->iNum;
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryGetString

    \Description
        Get a string entry from the registry

    \Input  *pEntryName - the registry entry name
    \Input  *pBuf       - destination string
    \Input   iBufSize   - size of the destination string
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistryGetString(const char *pEntryName, char *pBuf, int32_t iBufSize)
{
    TesterRegistryEntryT *pEntry;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pBuf == NULL) || (iBufSize == 0))
    {
        ZPrintf("WARNING: TesterRegistryGet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    ds_memclr(pBuf, iBufSize);
    if ((pEntry = _TesterRegistryFindEntry(pEntryName)) == NULL)
    {
        // no entry found
        return(TESTERREGISTRY_ERROR_NOSUCHENTRY);
    }

    ds_strnzcpy(pBuf, pEntry->strBuffer, iBufSize-1);
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryPrint

    \Description
        Print all the members of the registry to the console

    \Input  None
    
    \Output None

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryPrint(void)
{
    if(_TesterRegistry->iNumEntries == 0)
    {
        ZPrintf("REGISTRY: SIZE=%d {<empty>}\n",_TesterRegistry->iMaxEntries);
    }
    else
    {
        int32_t iEntryIndex;

        ZPrintf("REGISTRY: SIZE=%d :\n",_TesterRegistry->iMaxEntries);
        for (iEntryIndex = 0; iEntryIndex < _TesterRegistry->iNumEntries; iEntryIndex += 1)
        {
            const TesterRegistryEntryT *pEntry = &_TesterRegistry->pReg[iEntryIndex];
            ZPrintf("REGISTRY: {name: %s, ptr: 0x%08x, num: %d, str: %s}\n",
                pEntry->strName, pEntry->uPtr, pEntry->iNum, pEntry->strBuffer);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function TesterRegistryDestroy

    \Description
        Destroy the registry and free all associated memory.

    \Input  None
    
    \Output None

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryDestroy(void)
{
    ZMemFree(_TesterRegistry);
    _TesterRegistry = NULL;
}

