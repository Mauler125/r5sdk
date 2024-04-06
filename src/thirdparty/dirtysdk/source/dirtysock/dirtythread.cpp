/*H**************************************************************************************/
/*!
    \File    dirtythread.cpp

    \Description
        Provide threading library functions for use by network layer code.

    \Copyright
        Copyright (c) Electronic Arts 2017

    \Version 09/27/17 (eesponda)
*/
/**************************************************************************************H*/

/*** Include files *********************************************************************/

#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_thread.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/dirtysock/dirtythread.h"

/*** Type Definitions ******************************************************************/

// data about your thread
typedef struct DirtyThreadT
{
    DirtyRunnableFunctionT *pFunction;      //!< the function that implements the guts of the thread
    void *pUserData;                        //!< user data passed along with the function

    int32_t iMemGroup;                      //!< memgroup identifier
    void *pMemGroupUserData;                //!< memgroup userdata
} DirtyThreadT;

struct DirtyConditionRefT
{
    EA::Thread::Condition Condition;    //!< condition state, must come first

    int32_t iMemGroup;                  //!< memory group identifier
    void *pMemGroupUserdata;            //!< user data passed along with allocation
};

/*** Private Functions *****************************************************************/

/*F**************************************************************************************/
/*!
    \Function _DirtyThreadWrapper

    \Description
        Wrapper function that allows us to log and cleanup thread state

    \Input *pUserData   - our threading state

    \Output
        intptr_t        - thread result

    \Version 09/28/2017 (eesponda)
*/
/***************************************************************************************F*/
EA_DISABLE_VC_WARNING(4702)
static intptr_t _DirtyThreadWrapper(void *pUserData)
{
    DirtyThreadT *pThread = (DirtyThreadT *)pUserData;

    // run the thread function
    pThread->pFunction(pThread->pUserData);

    // clean up and return
    DirtyMemFree(pThread, DIRTYTHREAD_MEMID, pThread->iMemGroup, pThread->pMemGroupUserData);

    return(0);
}
EA_RESTORE_VC_WARNING()

/*** Public Functions *******************************************************************/


/*F**************************************************************************************/
/*!
    \Function DirtyThreadCreate

    \Description
        Allocate the thread state and start a thread, running it in our own wrapper function

    \Input *pFunction   - function that is run in the thread
    \Input *pUserData   - user data passed along with that function
    \Input *pConfig     - addtional configuration needed

    \Output
        int32_t         - zero=success, negative=failure

    \Version 09/28/2017 (eesponda)
*/
/***************************************************************************************F*/
int32_t DirtyThreadCreate(DirtyRunnableFunctionT *pFunction, void *pUserData, const DirtyThreadConfigT *pConfig)
{
    /* we create the thread object on the stack here as it is not tied to the lifetime of the thread
       just facilitates its creation */
    EA::Thread::Thread Thread;
    EA::Thread::ThreadParameters Parameters;
    DirtyThreadT *pThread;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query memgroup info
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate thread state
    if ((pThread = (DirtyThreadT *)DirtyMemAlloc(sizeof(*pThread), DIRTYTHREAD_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(-1);
    }
    ds_memclr(pThread, sizeof(*pThread));
    pThread->pFunction = pFunction;
    pThread->pUserData = pUserData;
    pThread->iMemGroup = iMemGroup;
    pThread->pMemGroupUserData = pMemGroupUserData;

    Parameters.mnAffinityMask = (pConfig->iAffinity > 0) ? pConfig->iAffinity : EA::Thread::kThreadAffinityMaskAny;
    Parameters.mnPriority     = pConfig->iPriority;
    Parameters.mnProcessor    = EA::Thread::kProcessorAny;
    Parameters.mpName         = pConfig->pName;

    // start the thread
    if (Thread.Begin(_DirtyThreadWrapper, pThread, &Parameters) == EA::Thread::kThreadIdInvalid)
    {
        DirtyMemFree(pThread, DIRTYTHREAD_MEMID, iMemGroup, pMemGroupUserData);
        return(-2);
    }
    return(0);
}

/*F**************************************************************************************/
/*!
    \Function DirtyThreadGetThreadId

    \Description
        Gets the thread identifier that can be used for logging purposes

    \Input *pBuffer - [out] the output buffer for the thread id
    \Input iBufSize - size of the output buffer

    \Output
        const char *- pointer to the output buffer passed in

    \Version 02/04/2020 (eesponda)
*/
/***************************************************************************************F*/
const char *DirtyThreadGetThreadId(char *pBuffer, int32_t iBufSize)
{
    return(ds_strnzcpy(pBuffer, EAThreadSysThreadIdToString(EA::Thread::GetSysThreadId()), iBufSize));
}

/*F**************************************************************************************/
/*!
    \Function DirtyConditionCreate

    \Description
        Creates a conditional variable with the given name

    \Input *pName           - name for the conditional

    \Output
        DirtyConditionRefT * - object ref on success, NULL otherwise

    \Version 09/27/2017 (eesponda)
*/
/***************************************************************************************F*/
DirtyConditionRefT *DirtyConditionCreate(const char *pName)
{
    DirtyConditionRefT *pCondition;
    int32_t iMemGroup;
    void *pMemGroupUserdata;
    EA::Thread::ConditionParameters Params;

    // query memgroup info
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // setup parameters
    ds_strnzcpy(Params.mName, pName, sizeof(Params.mName));
    Params.mbIntraProcess = true;

    // allocate the memory needed for condition state
    if ((pCondition = (DirtyConditionRefT *)DirtyMemAlloc(sizeof(*pCondition), DIRTYTHREAD_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        NetPrintf(("dirtythread: unable to allocate state for condition variable\n"));
        return(NULL);
    }
    ds_memclr(pCondition, sizeof(*pCondition));
    pCondition->iMemGroup = iMemGroup;
    pCondition->pMemGroupUserdata = pMemGroupUserdata;
    if (!pCondition->Condition.Init(&Params))
    {
        NetPrintf(("dirtythread: [%p] condition init returned false\n", pCondition));
        DirtyConditionDestroy(pCondition);
        return(NULL);
    }

    return(pCondition);
}

/*F**************************************************************************************/
/*!
    \Function DirtyConditionDestroy

    \Description
        Destroys the condition variable

    \Input *pCondition  - condition variable state

    \Version 09/27/2017 (eesponda)
*/
/***************************************************************************************F*/
void DirtyConditionDestroy(DirtyConditionRefT *pCondition)
{
    pCondition->Condition.~Condition();
    DirtyMemFree(pCondition, DIRTYTHREAD_MEMID, pCondition->iMemGroup, pCondition->pMemGroupUserdata);
}

/*F**************************************************************************************/
/*!
    \Function DirtyConditionWait

    \Description
        Waits on a condition given a crit (mutex)

    \Input *pCondition  - condition variable state
    \Input *pCrit       - mutex used for wait

    \Version 09/27/2017 (eesponda)
*/
/***************************************************************************************F*/
void DirtyConditionWait(DirtyConditionRefT *pCondition, NetCritT *pCrit)
{
    // we can cast here since we know that the first field in crit is the mutex
    EA::Thread::Mutex *pMutex = (EA::Thread::Mutex *)pCrit->pData;
    pCondition->Condition.Wait(pMutex);
}

/*F**************************************************************************************/
/*!
    \Function DirtyConditionSignal

    \Description
        Signals a condition

    \Input *pCondition  - condition variable state

    \Output
        uint8_t         - TRUE if successful, FALSE otherwsie

    \Version 09/27/2017 (eesponda)
*/
/***************************************************************************************F*/
uint8_t DirtyConditionSignal(DirtyConditionRefT *pCondition)
{
    return(pCondition->Condition.Signal());
}
