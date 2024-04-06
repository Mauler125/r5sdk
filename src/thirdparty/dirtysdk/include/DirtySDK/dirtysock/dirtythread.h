/*H**************************************************************************************/
/*!
    \File    dirtythread.h

    \Description
        Provide threading library functions for use by network layer code.

    \Copyright
        Copyright (c) Electronic Arts 2017

    \Version 09/27/17 (eesponda)
*/
/**************************************************************************************H*/

#ifndef _dirtythread_h
#define _dirtythread_h

/*!
\Moduledef DirtyThread DirtyThread
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions ******************************************************************/

// configuration for the thread
typedef struct DirtyThreadConfigT
{
    int32_t iPriority;  //!< priority of the thread, platform dependent
    int32_t iAffinity;  //!< affinity mask
    const char *pName;  //!< name of the thread
    int32_t iVerbosity; //!< verbosity of logging information (deprecated)
} DirtyThreadConfigT;

// function that gets run on the thread
typedef void (DirtyRunnableFunctionT)(void* pUserData);

// opaque module ref
typedef struct DirtyConditionRefT DirtyConditionRefT;

// forward declaration
typedef struct NetCritT NetCritT;

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// start the thread
DIRTYCODE_API int32_t DirtyThreadCreate(DirtyRunnableFunctionT *pFunction, void *pUserData, const DirtyThreadConfigT *pConfig);

// get thread id
DIRTYCODE_API const char *DirtyThreadGetThreadId(char *pBuffer, int32_t iBufSize);

// create a condition variable with name
DIRTYCODE_API DirtyConditionRefT *DirtyConditionCreate(const char *pName);

// destroy the condition
DIRTYCODE_API void DirtyConditionDestroy(DirtyConditionRefT *pCondition);

// wait for a condition
DIRTYCODE_API void DirtyConditionWait(DirtyConditionRefT *pCondition, NetCritT *pCrit);

// signal the condition
DIRTYCODE_API uint8_t DirtyConditionSignal(DirtyConditionRefT *pCondition);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtythread_h
