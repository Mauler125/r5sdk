/*H********************************************************************************/
/*!
    \File weblog.h

    \Description
        Captures DirtySDK debug output and posts it to a webserver where the output
        can be retrieved.  This is useful when debugging on a system with no
        debugging capability or in a "clean room" environment, for example.

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 05/06/2008 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _weblog_h
#define _weblog_h

/*!
\Moduledef WebLog WebLog
\Modulemember Misc
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct WebLogRefT WebLogRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API WebLogRefT *WebLogCreate(int32_t iBufSize);

// configure the module
DIRTYCODE_API void WebLogConfigure(WebLogRefT *pWebLog, const char *pServer, const char *pUrl);

// start web log logging
DIRTYCODE_API void WebLogStart(WebLogRefT *pWebLog);

// stop web log logging
DIRTYCODE_API void WebLogStop(WebLogRefT *pWebLog);

// destroy the module
DIRTYCODE_API void WebLogDestroy(WebLogRefT *pWebLog);

// weblog netprintf hook
DIRTYCODE_API int32_t WebLogDebugHook(void *pUserData, const char *pText);

// print to weblog buffer
DIRTYCODE_API void WebLogPrintf(WebLogRefT *pWebLog, const char *pFormat, ...);

// control weblog behavior
DIRTYCODE_API int32_t WebLogControl(WebLogRefT *pWebLog, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// update the weblog module
DIRTYCODE_API void WebLogUpdate(WebLogRefT *pWebLog);

#ifdef __cplusplus
}
#endif

//@}

#endif // _weblog_h

