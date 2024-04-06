/*H********************************************************************************/
/*!
    \File voippriv.h

    \Description
        Header for private VoIP functions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
    \Version 1.1 11/18/2008 (mclouatre) Adding user data concept to mem group support
*/
/********************************************************************************H*/

#ifndef _voippriv_h
#define _voippriv_h

/*** Include files ****************************************************************/
#include "DirtySDK/dirtysock/netconn.h"

/*** Defines **********************************************************************/
#define VOIPUSER_FLAG_CROSSPLAY          (1<<0) //!< is this voip user running in crossplay mode?

// enumerate supported platforms
// always add new platforms at the end and never recycle values
typedef enum VoipPlatformTypeE
{
    VOIP_PLATFORM_LINUX = 1,  // linux is a stub for the stress client
    VOIP_PLATFORM_PC = 2,
    VOIP_PLATFORM_PS4 = 3,
    VOIP_PLATFORM_XBOXONE = 4
} VoipPlatformTypeE;

// used to identify our local platform
#if defined(DIRTYCODE_LINUX)
#define VOIP_LOCAL_PLATFORM VOIP_PLATFORM_LINUX
#elif defined(DIRTYCODE_PC)
#define VOIP_LOCAL_PLATFORM VOIP_PLATFORM_PC
#elif defined(DIRTYCODE_PS4)
#define VOIP_LOCAL_PLATFORM VOIP_PLATFORM_PS4
#elif defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK) // $$todo$$ should we treat xbsx as a separate platform?
#define VOIP_LOCAL_PLATFORM VOIP_PLATFORM_XBOXONE
#endif

/*** Macros ***********************************************************************/

//! copy VoipUserTs
#define VOIP_CopyUser(_pUser1, _pUser2) (ds_memcpy_s(_pUser1, sizeof(*_pUser1), _pUser2, sizeof(*_pUser2)))

//! clear VoipUserT
#define VOIP_ClearUser(_pUser1)         (ds_memclr(_pUser1, sizeof(*_pUser1)))

//! return if VoipUserT is NULL or not
#define VOIP_NullUser(_pUser1)          ((_pUser1)->AccountInfo.iPersonaId == 0)

//! compare VoipUserTs
#define VOIP_SameUser(_pUser1, _pUser2) ((_pUser1)->AccountInfo.iPersonaId == (_pUser2)->AccountInfo.iPersonaId)

/*** Type Definitions *************************************************************/

typedef struct VoipUserT
{
    NetConnAccountInfoT AccountInfo; //!< account info
    uint32_t uFlags;                 //!< a bit field contaning VOIPUSER_FLAG_*
    VoipPlatformTypeE  ePlatform;    //!< what platform is this user running on
} VoipUserT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // _voippriv_h

