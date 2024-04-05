/*H********************************************************************************/
/*!

    \File    dirtylib.cpp

    \Description
        Platform specific support library for network code. Suppplies
        simple time, memory, and semaphore functions.

    \Copyright
        Copyright (c) Electronic Arts 2002-2018.  ALL RIGHTS RESERVED.

    \Version    01/02/02 (eesponda) Initial C++ version ported to use EAThread
*/
/********************************************************************************H*/

#include "eathread/eathread_mutex.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/dirtysock/dirtylib.h"

/*** Type Definitions ************************************************************/

// critical section state
struct NetCritPrivT
{
    EA::Thread::Mutex Mutex;    //!< mutex state, must come first

    uint8_t bEnabled;           //!< controls if this lock is enabled
    uint8_t _pad[3];

    int32_t iMemGroup;          //!< memory group identifier
    void *pMemGroupUserdata;    //!< user data passed along with allocation
};

/*** Variables *******************************************************************/

// global critical section
static NetCritPrivT _NetLib_GlobalCrit;

// sets the functions to no-op in this situation
#if defined(DIRTYCODE_LINUX)
extern uint8_t _NetLib_bSingleThreaded;
#else
static uint8_t _NetLib_bSingleThreaded = FALSE;
#endif

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetCritInit

    \Description
        Initialize a critical section for use. Allocates persistant storage
        for the lifetime of the critical section if not using the global crit.

    \Input *pCrit       - critical section marker
    \Input *pCritName   - name of the critical section

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/26/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t NetCritInit(NetCritT *pCrit, const char *pCritName)
{
    return(NetCritInit2(pCrit, pCritName, NETCRIT_OPTION_NONE));
}

/*F********************************************************************************/
/*!
    \Function NetCritInit2

    \Description
        Initialize a critical section for use. Allocates persistant storage
        for the lifetime of the critical section if not using the global crit.

    \Input *pCrit       - critical section marker
    \Input *pCritName   - name of the critical section
    \Input uFlags       - NETCRIT_OPTIONS_* flag options to set on the crit

    \Output
        int32_t         - zero=success, negative=failure

    \Version 07/24/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t NetCritInit2(NetCritT *pCrit, const char *pCritName, uint32_t uFlags)
{
    NetCritPrivT *pPriv;
    EA::Thread::MutexParameters Params;
    ds_strnzcpy(Params.mName, pCritName, sizeof(Params.mName));
    Params.mbIntraProcess = true;

    // allocate memory if necessary
    if (pCrit != NULL)
    {
        int32_t iMemGroup;
        void *pMemGroupUserdata;

        // query memgroup info
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

        // allocate the memory needed for mutex state
        if ((pCrit->pData = (NetCritPrivT *)DirtyMemAlloc(sizeof(*pCrit->pData), DIRTYTHREAD_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
        {
            return(-1);
        }
        ds_memclr(pCrit->pData, sizeof(*pCrit->pData));
        pCrit->pData->iMemGroup = iMemGroup;
        pCrit->pData->pMemGroupUserdata = pMemGroupUserdata;
    }
    /* we always clear the single-thread enable option on the global crit. it would be counter productive if it was
       enabled when we are in single-threaded mode */
    else
    {
        uFlags &= ~NETCRIT_OPTION_SINGLETHREADENABLE;
    }

    pPriv = (pCrit ? pCrit->pData : &_NetLib_GlobalCrit);
    pPriv->Mutex.Init(&Params);
    pPriv->bEnabled = !_NetLib_bSingleThreaded || ((uFlags & NETCRIT_OPTION_SINGLETHREADENABLE) == NETCRIT_OPTION_SINGLETHREADENABLE);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function NetCritKill

    \Description
        Release resources and destroy critical section.

    \Input *pCrit   - critical section marker

    \Version 09/26/2017 (eesponda)
*/
/********************************************************************************F*/
void NetCritKill(NetCritT *pCrit)
{
    EA::Thread::Mutex *pMutex = (pCrit ? &pCrit->pData->Mutex : &_NetLib_GlobalCrit.Mutex);
    pMutex->~Mutex();

    // if we are not using the global crit free the memory
    if (pCrit != NULL)
    {
        NetCritPrivT *pPriv = pCrit->pData;
        DirtyMemFree(pPriv, DIRTYTHREAD_MEMID, pPriv->iMemGroup, pPriv->pMemGroupUserdata);
        pCrit->pData = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritEnter

    \Description
        Enter a critical section, blocking if needed.

    \Input *pCrit   - critical section marker

    \Version 09/26/2017 (eesponda)
*/
/********************************************************************************F*/
void NetCritEnter(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (pCrit ? pCrit->pData : &_NetLib_GlobalCrit);
    if (pPriv->bEnabled)
    {
        pPriv->Mutex.Lock();
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritTry

    \Description
        Attempt to gain access to critical section. Always returns immediately
        regadless of access status. A thread that already has access to a critical
        section can always receive repeated access to it.

    \Input *pCrit   - critical section marker

    \Output
        int32_t     - zero=unable to get access, non-zero=access granted

    \Version 09/26/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t NetCritTry(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (pCrit ? pCrit->pData : &_NetLib_GlobalCrit);
    if (pPriv->bEnabled)
    {
        return(pPriv->Mutex.Lock(EA::Thread::kTimeoutImmediate) > 0);
    }
    else
    {
        return(1);
    }
}

/*F********************************************************************************/
/*!
    \Function NetCritLeave

    \Description
        Leave a critical section. Must be called once for every NetCritEnter (or
        successful NetCritTry).

    \Input *pCrit   - critical section marker

    \Version 09/26/2017 (eesponda)
*/
/********************************************************************************F*/
void NetCritLeave(NetCritT *pCrit)
{
    NetCritPrivT *pPriv = (pCrit ? pCrit->pData : &_NetLib_GlobalCrit);
    if (pPriv->bEnabled)
    {
        pPriv->Mutex.Unlock();
    }
}

