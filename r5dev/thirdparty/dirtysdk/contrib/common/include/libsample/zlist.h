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

#ifndef _zlist_h
#define _zlist_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define ZLIST_ERROR_NONE             (0)    //!< no error (success)
#define ZLIST_ERROR_NULLPOINTER     (-1)    //!< a null pointer ref was used
#define ZLIST_ERROR_FULL            (-2)    //!< sending/receiving list is full (msg dropped)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ZListT ZListT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a list object
DIRTYCODE_API ZListT *ZListCreate(int32_t iNumEntries, int32_t iEntrySize);

// add an entry to the back of a data list
DIRTYCODE_API int32_t ZListPushBack(ZListT *pList, void *pEntry);

// get an entry off the front of a data list
DIRTYCODE_API int32_t ZListPopFront(ZListT *pList, void *pEntry);

// examine the front entry of a data list
DIRTYCODE_API void *ZListPeekFront(ZListT *pList);

// erase and entire list
DIRTYCODE_API void ZListClear(ZListT *pList);

// destroy a list object
DIRTYCODE_API void ZListDestroy(ZListT *pList);

#ifdef __cplusplus
};
#endif

#endif // _zlist_h

