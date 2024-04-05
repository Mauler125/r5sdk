/*H********************************************************************************/
/*!
    \File zlist.h

    \Description
        Generic list module for samples to use.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/26/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "zmem.h"
#include "zlib.h"
#include "zlist.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

struct ZListT
{
    uint32_t iNumEntries;   //!< number of entries in the list
    uint32_t iEntrySize;    //!< size of each entry
    uint32_t uHead;         //!< index of input list head entry
    uint32_t uTail;         //!< index of input list tail entry
    void *pData;                //!< data
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ZListCreate

    \Description
        Create a list object

    \Input iMaxEntries - maximum number of entries in the list
    \Input iEntrySize  - maximum size of each entry in the list
    
    \Output ZListT * - pointer to the created list

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
ZListT *ZListCreate(int32_t iNumEntries, int32_t iEntrySize)
{
    ZListT *pList;

    // check for errors
    if((iNumEntries <= 0) || (iEntrySize <= 0))
    {
        ZPrintf("Could not create list with [%d] entries of size [%d] total size [%d]\n",
            iNumEntries, iEntrySize, iNumEntries * iEntrySize);
        return(NULL);
    }

    // create the list
    pList = (ZListT *)ZMemAlloc(sizeof(ZListT));
    ds_memclr(pList, sizeof(*pList));
    pList->iEntrySize = iEntrySize;
    pList->iNumEntries = iNumEntries;
    pList->pData = (ZListT *)ZMemAlloc(iNumEntries * iEntrySize);
    return(pList);
}


/*F********************************************************************************/
/*!
    \Function ZListPushBack

    \Description
        Add and entry to the back of a data list

    \Input pList  - pointer to the list to use
    \Input pEntry - entry to add (will be copied in)
    
    \Output int32_t - 0 for success, error code otherwise

    \TODO
        The current implementation doesn't wrap the queue, so if the head
        starts chasing the tail but doesn't catch up, a large amount of the
        buffer space can end up being wasted.  Either the queue needs to be
        modified to wrap, or the buffer memory shifted to allow the head to
        be reset to zero without catching the tail.

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ZListPushBack(ZListT *pList, void *pEntry)
{
    char *pDest;

    // check to make sure we got an entry
    if ((pEntry == NULL) || (pList == NULL))
    {
        return(ZLIST_ERROR_NULLPOINTER);
    }

    // see if the list is full 
    if (pList->uTail >= pList->iNumEntries)
    {
        return(ZLIST_ERROR_FULL);
    }

    // insert into the list
    pDest = (char *)pList->pData + (pList->uTail * pList->iEntrySize);
    ds_memcpy(pDest, pEntry, pList->iEntrySize);
    // increment the tail pointer
    pList->uTail++;

    // done
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ZListPopFront

    \Description
        Get an entry off the front of a data list

    \Input pList - pointer to the list to destroy
    \Input pEntry - destination for the entry to get (will be copied in), NULL to discard data
    
    \Output int32_t - <0 for error, 0 for no data left, >0 for amount of data left

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t ZListPopFront(ZListT *pList, void *pEntry)
{
    uint32_t uAmtLeft;
    char *pSrc;

    // check to make sure we got an entry
    if (pList == NULL)
    {
        return(ZLIST_ERROR_NULLPOINTER);
    }

    // see if the list is empty
    uAmtLeft = pList->uTail - pList->uHead;
    if (uAmtLeft == 0)
    {
        // no error - list is just empty
        return(0);
    }
    else
    {
        // only copy data if we have a container for it
        if (pEntry)
        {
            // copy from the list into the new location
            pSrc = (char *)pList->pData + (pList->uHead * pList->iEntrySize);
            ds_memcpy(pEntry, (void *)pSrc, pList->iEntrySize);
        }
        pList->uHead++;
        // test for empty list situation
        if (pList->uHead == pList->uTail)
        {
            // empty list - reset
            pList->uHead = 0;
            pList->uTail = 0;
        }
    }

    // done
    return(uAmtLeft);
}


/*F********************************************************************************/
/*!
    \Function ZListPeekFront

    \Description
        Examine the front entry of a data list

    \Input pList - pointer to the list to destroy
    
    \Output void * - pointer to the first entry, NULL if empty

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
void *ZListPeekFront(ZListT *pList)
{
    char *pSrc;

    // check for errors
    if(pList == NULL)
    {
        return(NULL);
    }

    // if list is empty, return NULL
    if (pList->uHead == pList->uTail)
    {
        return(NULL);
    }

    // otherwise return a pointer to the data in question
    pSrc = (char *)pList->pData + (pList->uHead * pList->iEntrySize);
    return(pSrc);
}


/*F********************************************************************************/
/*!
    \Function ZListClear

    \Description
        Clear the entire list

    \Input pList - pointer to the list to destroy
    
    \Output None

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
void ZListClear(ZListT *pList)
{
    if(pList)
    {
        ds_memclr(pList->pData, pList->iNumEntries * pList->iEntrySize);
        pList->uHead = 0;
        pList->uTail = 0;
    }
}


/*F********************************************************************************/
/*!
    \Function ZListDestroy

    \Description
        Destroy a list object and free all associated memory

    \Input pList - pointer to the list to destroy
    
    \Output None

    \Version 04/26/2005 (jfrank)
*/
/********************************************************************************F*/
void ZListDestroy(ZListT *pList)
{
    if (pList)
    {
        if (pList->pData)
        {
            ZMemFree(pList->pData);
        }

        ZMemFree(pList);
    }
}

