/*H*************************************************************************************/
/*!
    \File dirtysessionmanager.h

    \Description
        DirtySessionManager handles the creation, joinning and leaving of
        a session. Offers mechanism to encode and decode the session, and does some
        session flags management

    \Notes

    \Copyright
        Copyright (c) Electronic Arts 2003-2007

    \Version 1.0 11/03/2003 (jbrookes) First Version
    \Version 2.0 11/04/2007 (jbrookes) Removed from ProtoMangle namespace, cleanup
    \Version 2.2 10/26/2009 (mclouatre) Renamed from core/include/dirtysessionmanager.h to core/include/xenon/dirtysessionmanagerxenon.h
    \Version 2.3 03/26/2013 (cvienneau) Renamed from core/include/xenon/dirtysessionmanagerxenon.h to core/include/dirtysessionmanager.h
*/
/*************************************************************************************H*/

#ifndef _dirtysessionmanager_h
#define _dirtysessionmanager_h

/*!
\Moduledef DirtySessionManager DirtySessionManager
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyaddr.h"

/*** Defines ***************************************************************************/

#define DIRTYSESSIONMANAGER_FLAG_PUBLICSLOT          (1)
#define DIRTYSESSIONMANAGER_FLAG_PRIVATESLOT         (2)

#if defined(DIRTYCODE_PS4)
#define DIRTY_SESSION_GAME_MODE_LENGTH               257
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DirtySessionManagerRefT DirtySessionManagerRefT;
#if DIRTYCODE_DEBUG
//! true skill values struct
typedef struct DirtySessionManagerTrueSkillRefT      //!< true skill struct used for debugging
{
    double dMu;
    double dSigma;
} DirtySessionManagerTrueSkillRefT;
#endif

#if defined(DIRTYCODE_PS4)
//!< data that we control within the binary blob portion of the session
typedef struct DirtySessionManagerBinaryHeaderT
{
    int64_t iLobbyId;
    int64_t iGameType;
} DirtySessionManagerBinaryHeaderT;

//!< data that we control within the changeable binary blob portion of the session
typedef struct DirtySessionManagerChangeableBinaryHeaderT
{
    char strGameMode[DIRTY_SESSION_GAME_MODE_LENGTH];
} DirtySessionManagerChangeableBinaryHeaderT;
#endif

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
DirtySessionManagerRefT *DirtySessionManagerCreate(void);

// destroy the module and release its state
void DirtySessionManagerDestroy(DirtySessionManagerRefT *pRef);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void DirtySessionManagerUpdate(DirtySessionManagerRefT *pRef);

// join one (or many) remote player(s) by specifying the session and type of slot to use
void DirtySessionManagerConnect(DirtySessionManagerRefT *pRef, const char **pSessID, uint32_t *pSlot, uint32_t uCount);

// dirtysessionmanager control
int32_t DirtySessionManagerControl(DirtySessionManagerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue);

// get module status based on selector
int32_t DirtySessionManagerStatus(DirtySessionManagerRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// get module status based on selector
int32_t DirtySessionManagerStatus2(DirtySessionManagerRefT *pRef, int32_t iSelect, int32_t iValue, int32_t iValue2, int32_t iValue3, void *pBuf, int32_t iBufSize);

// encode
void DirtySessionManagerEncodeSession(char *pBuffer, int32_t iBufSize, const void *pSessionInfo);

// decode
void DirtySessionManagerDecodeSession(void *pSessionInfo, const char *pBuffer);

// create the session (previously the 'sess' control selector)
int32_t DirtySessionManagerCreateSess(DirtySessionManagerRefT *pRef, uint32_t bRanked, uint32_t *uUserFlags, const char *pSession, DirtyAddrT *pLocalAddrs);

#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtysessionmanager_h


