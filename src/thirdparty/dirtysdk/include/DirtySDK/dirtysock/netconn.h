/*H*************************************************************************************/
/*!
    \File    netconn.h

    \Description
        Provides network setup and teardown support. Does not actually create any
        kind of network connections.

    \Copyright
        Copyright (c) Electronic Arts 2001-2009

    \Version 03/12/2001 (gschaefer) First Version
*/
/*************************************************************************************H*/

#ifndef _netconn_h
#define _netconn_h

/*!
\Moduledef NetConnDefs NetConnDefs
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/netconndefs.h"

/*** Defines ***************************************************************************/

//! maximum number of local users
#if defined(DIRTYCODE_XBOXONE)
    #define NETCONN_MAXLOCALUSERS    (16)
#elif defined(DIRTYCODE_NX)
    #define NETCONN_MAXLOCALUSERS    (8)
#else
    #define NETCONN_MAXLOCALUSERS    (4)
#endif


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct NetConnUserDataT
{
    char    strName[16];
    void    *pRawData1;
    void    *pRawData2;
} NetConnUserDataT;

//! account structure
typedef struct NetConnAccountInfoT
{
    int64_t   iAccountId; //!< the EA account Id of the user
    int64_t   iPersonaId; //!< the EA persona Id of the user
} NetConnAccountInfoT;

#if defined(DIRTYCODE_PS4)
typedef void (NetConnNpStateCallbackT)(int32_t /* SceUserServiceUserId */ userId, int32_t /* SceNpState */ state, void* pUserData);
#endif

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// bring the network connection module to life
DIRTYCODE_API int32_t NetConnStartup(const char *pParams);

// query the list of available connection configurations
DIRTYCODE_API int32_t NetConnQuery(const char *pDevice, NetConfigRecT *pList, int32_t iSize);

// bring the networking online with a specific configuration
DIRTYCODE_API int32_t NetConnConnect(const NetConfigRecT *pConfig, const char *pParms, int32_t iData);

// set module behavior based on input selector
DIRTYCODE_API int32_t NetConnControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue, void *pValue2);

// check general network connection status (added param)
DIRTYCODE_API int32_t NetConnStatus(int32_t iKind, int32_t iData, void *pBuf, int32_t iBufSize);

// return MAC address in textual form
DIRTYCODE_API const char *NetConnMAC(void);

// take down the network connection
DIRTYCODE_API int32_t NetConnDisconnect(void);

// shutdown the network code and return to idle state
DIRTYCODE_API int32_t NetConnShutdown(uint32_t uShutdownFlags);

// return elapsed time in milliseconds
DIRTYCODE_API uint32_t NetConnElapsed(void);

// sleep the application (burn cycles) for some number of milliseconds
DIRTYCODE_API void NetConnSleep(int32_t iMilliSecs);

// add an idle handler that will get called periodically
DIRTYCODE_API int32_t NetConnIdleAdd(void (*proc)(void *data, uint32_t tick), void *data);

// remove a previously added idle handler
DIRTYCODE_API int32_t NetConnIdleDel(void (*proc)(void *data, uint32_t tick), void *data);

// provide "life" to the network code
DIRTYCODE_API void NetConnIdle(void);

// shut down netconn idle handler
// NOTE: this is meant for internal use only, and should not be called by applications directly
DIRTYCODE_API void NetConnIdleShutdown(void);

// Enable or disable the timing of netconnidles
DIRTYCODE_API void NetConnTiming(uint8_t uEnableTiming);

typedef void (UserInfoCallbackT)(NetConnUserDataT *pUserDataT, void *pData);

// get the unique machine id
DIRTYCODE_API uint32_t NetConnMachineId(void);

// set the unique machine id
DIRTYCODE_API void NetConnSetMachineId(uint32_t uMachineId);

#if DIRTYCODE_LOGGING
DIRTYCODE_API void NetConnMonitorValue(const char* pName, int32_t iValue);
#endif

// copy a startup parameter
DIRTYCODE_API int32_t NetConnCopyParam(char *pDst, int32_t iDstLen, const char *pParamName, const char *pSrc, const char *pDefault);

// create dirtycert module
DIRTYCODE_API int32_t NetConnDirtyCertCreate(const char *pParams);

// translate netconn environment to string
DIRTYCODE_API const char *NetConnGetEnvStr(void);

#if defined(DIRTYCODE_PS4)
DIRTYCODE_API void NetConnRegisterNpStateCallback(NetConnNpStateCallbackT *pCallback, void *pUserData);
#endif

#ifdef __cplusplus
}

// forward declare IEAUser so as not to create a dependency
namespace EA { namespace User { class IEAUser; } }

// use this function to tell netconn about a newly detected local user on the local console
DIRTYCODE_API int32_t NetConnAddLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);

// use this function to tell netconn about a removed local user on the local console
// in cases where the index is unknown, pass -1 and we will do an internal query
DIRTYCODE_API int32_t NetConnRemoveLocalUser(int32_t iLocalUserIndex, const EA::User::IEAUser *pLocalUser);
#endif // __cplusplus

//@}

#endif // _netconn_h

