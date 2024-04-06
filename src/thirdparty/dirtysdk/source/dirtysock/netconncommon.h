/*H********************************************************************************/
/*!
    \File netconncommon.h

    \Description
        Cross-platform netconn data types and private functions.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 05/21/2009 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _netconncommon_h
#define _netconncommon_h

/*** Include files ****************************************************************/
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **********************************************************************/

// initial size of external cleanup list (in number of entries)
#define NETCONN_EXTERNAL_CLEANUP_LIST_INITIAL_CAPACITY (12)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! external cleanup callback function prototype
typedef int32_t(*NetConnExternalCleanupCallbackT)(void *pNetConnExternalCleanupData);

typedef struct NetConnExternalCleanupEntryT
{
    void *pCleanupData;                         //!< pointer to data to be passed to the external cleanup callback
    NetConnExternalCleanupCallbackT  pCleanupCb;//!< external cleanup callback
} NetConnExternalCleanupEntryT;

typedef struct NetConnCommonRefT
{
    // module memory group
    int32_t              iMemGroup;                             //!< module mem group id
    void                 *pMemGroupUserData;                    //!< user data associated with mem group

    int32_t              iDebugLevel;
    
    int32_t              iExternalCleanupListMax;               //!< maximum number of entries in the array
    int32_t              iExternalCleanupListCnt;               //!< number of valid entries in the array
    NetConnExternalCleanupEntryT *pExternalCleanupList;         //!< pointer to an array of entries pending external cleanup completion

    int32_t              iRefCount;                             //!< module reference counter

    NetConnAccountInfoT  aAccountInfo[NETCONN_MAXLOCALUSERS];  //!< account info array 

    NetCritT             crit;
} NetConnCommonRefT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// handle common shutdown functionality
void NetConnCommonShutdown(NetConnCommonRefT *pCommonRef);

// handle common startup functionality
int32_t NetConnCommonStartup(int32_t iNetConnRefSize, const char *pParams, NetConnCommonRefT **pRef);

// add an entry to the list of external module pending successful cleanup
int32_t NetConnCommonAddToExternalCleanupList(NetConnCommonRefT *pCommonRef, NetConnExternalCleanupCallbackT pCleanupCb, void *pCleanupData);

// walk external cleanup list and try to destroy each individual entry
int32_t NetConnCommonProcessExternalCleanupList(NetConnCommonRefT *pCommonRef);

// decrement and verify the reference count for shutdown
int32_t NetConnCommonCheckRef(NetConnCommonRefT *pCommonRef);

// set module behavior based on input selector
int32_t NetConnCommonControl(NetConnCommonRefT *pCommonRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2);

// check general network connection status (added param)
int32_t NetConnCommonStatus(NetConnCommonRefT *pCommonRef, int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize);
#ifdef __cplusplus
}
#endif

#endif // _netconcommon_h

