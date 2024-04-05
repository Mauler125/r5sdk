/*H********************************************************************************/
/*!
    \File netgamedistserv.h

    \Description
        Server module to handle 2+ NetGameDist connections in a client/server
        architecture.

    \Copyright
        Copyright (c) 2007 Electronic Arts

    \Version 02/01/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _netgamedistserv_h
#define _netgamedistserv_h

/*!
\Moduledef NetGameDistServ NetGameDistServ
\Modulemember Game
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct NetGameDistServT NetGameDistServT;

//! logging function type
typedef int32_t (NetGameDistServLoggingCbT)(const char *pText, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
DIRTYCODE_API NetGameDistServT *NetGameDistServCreate(int32_t iMaxClients, int32_t iVerbosity);

// destroy the module state
DIRTYCODE_API void NetGameDistServDestroy(NetGameDistServT *pDistServ);

// add a client to module
DIRTYCODE_API int32_t NetGameDistServAddClient(NetGameDistServT *pDistServ, int32_t iClient, NetGameLinkRefT *pLinkRef, const char *pClientName);

// del a client from module
DIRTYCODE_API int32_t NetGameDistServDelClient(NetGameDistServT *pDistServ, int32_t iClient);

// notify that a client disconnected
DIRTYCODE_API int32_t NetGameDistServDiscClient(NetGameDistServT *pDistServ, int32_t iClient);

// update a client
DIRTYCODE_API int32_t NetGameDistServUpdateClient(NetGameDistServT *pDistServ, int32_t iClient);

// update the module (must be called at fixed rate)
DIRTYCODE_API void NetGameDistServUpdate(NetGameDistServT *pDistServ);

// whether the highwater mark changed, and the current highwater values.
DIRTYCODE_API uint8_t NetGameDistServHighWaterChanged(NetGameDistServT *pDistServ, int32_t* pHighWaterInputQueue, int32_t* pHighWaterOutputQueue);

// return the lastest error reported by netgamedist, for client iClient
DIRTYCODE_API char* NetGameDistServExplainError(NetGameDistServT *pDistServ, int32_t iClient);

// netgamedistserv control
DIRTYCODE_API int32_t NetGameDistServControl(NetGameDistServT *pDistServ, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// netgamedistserv status
DIRTYCODE_API int32_t NetGameDistServStatus(NetGameDistServT *pDistServ, int32_t iSelect, void *pBuf, int32_t iBufSize);

// netgamedistserv2 status
DIRTYCODE_API int32_t NetGameDistServStatus2(NetGameDistServT *pDistServ, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// set logging callback
DIRTYCODE_API void NetGameDistServSetLoggingCallback(NetGameDistServT *pDistServ, NetGameDistServLoggingCbT *pLoggingCb, void *pUserData);

#ifdef __cplusplus
}
#endif

//@}

#endif // _netgamedistserv_h

