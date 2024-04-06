/*H*************************************************************************************************/
/*!

    \File    voipdef.h

    \Description
        Definitions shared between Voip EE library and Voip IRX module.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002-2004.    ALL RIGHTS RESERVED.

    \Version    1.0        11/03/03 (JLB) First Version
    \Version    1.5        03/02/04 (JLB) VoIP 2.0 - API changes for future multiple channel support.

*/
/*************************************************************************************************H*/

#ifndef _voipdef_h
#define _voipdef_h

/*!
\Moduledef VoipDef VoipDef
\Modulemember Voip
*/
//@{

/*** Include files ********************************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **************************************************************************************/

//! current version of VoIP (maj.min.rev.patch)
#define VOIP_VERSION                (0x04000000)

//! suggested VOIP port
#define VOIP_PORT                   (6000)

//! special connection identifiers
#define VOIP_CONNID_NONE            (-1)
#define VOIP_CONNID_ALL             (-2)

//! maximum number of supported connections
#define VOIP_MAXCONNECT             (32)

//! VoIP conn status flags (use VoipGroupConnStatus to query)
#define VOIP_CONN_CONNECTED       (1)     //!< UDP connection was set up OK   
#define VOIP_CONN_BROADCONN       (2)     //!< is connection broadband?
#define VOIP_CONN_ACTIVE          (4)     //!< both sides have headsets & broadband and are exchanging voice traffic
#define VOIP_CONN_STOPPED         (8)     //!< connection attempt stopped

//! VoIP remote status flags (use VoipGroupRemoteUserStatus to query)
#define VOIP_REMOTE_USER_RECVVOICE       (1)     //!< voice packets being received
#define VOIP_REMOTE_USER_HEADSETOK       (2)     //!< the remote user's headset is setup

//! VoIP local status flags (use VoipLocalUserStatus to query)
#define VOIP_LOCAL_USER_UNUSED           (1)     //!< unused
#define VOIP_LOCAL_USER_HEADSETOK        (2)     //!< a compatible headset is connected and successfully initialized
                                                 //!< can only be set when both VOIP_LOCAL_USER_INPUTDEVICEOK and VOIP_LOCAL_USER_OUTPUTDEVICEOK are set
#define VOIP_LOCAL_USER_TALKING          (4)     //!< local user is talking (will be always set for codecs without VAD)
#define VOIP_LOCAL_USER_SENDVOICE        (8)     //!< voice packets being sent
#define VOIP_LOCAL_USER_INPUTDEVICEOK    (16)    //!< user has an inserted(xone/ps4) or selected(pc) input voice device
#define VOIP_LOCAL_USER_OUTPUTDEVICEOK   (32)    //!< user has an inserted(xone/ps4) or selected(pc) output voice device

//! Invalid user index (used on pc to indicate no user is active)
#define VOIP_INVALID_LOCAL_USER_INDEX  (-1)

#define VOIP_TALKTIMEOUT (100)    //!< clear talking flag after 1/10th sec of no voice

//! maximum number of local users on a single host
#if defined(DIRTYCODE_PS4)
#define VOIP_MAXLOCALUSERS (4)
#elif defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#define VOIP_MAXLOCALUSERS (16)
#else
#define VOIP_MAXLOCALUSERS (4)
#endif

#define VOIP_SHARED_REMOTE_INDEX (0xFF)
#define VOIP_SHARED_USER_INDEX VOIP_MAXLOCALUSERS
#define VOIP_SHARED_USER_VALUE (0xAAAAAAAA00000000)  //!< usually persona id is used to identify a user, in the case of a shared user this set of bits and a 32-bit uClientId is used
#define VOIP_SHARED_USER_MASK (0xFFFFFFFF00000000)  //!< this mask can be used in a test to see if the personaid is actually a shared user
#define VOIP_MAXLOCALUSERS_EXTENDED (VOIP_MAXLOCALUSERS + 1)

#define VOIP_MAX_CONCURRENT_CHANNEL (4)
#define VOIP_MAX_LOW_LEVEL_CONNS    (32)
#define VOIP_MAXLOCALCHANNEL        (64)

//! VoIP invalid flag
#define VOIP_FLAG_INVALID           (0x80000000)

//! callback event types
typedef enum VoipCbTypeE
{
    VOIP_CBTYPE_HSETEVENT,          //!< a headset has been inserted or removed
    VOIP_CBTYPE_FROMEVENT,          //!< VoipStatus('from') change
    VOIP_CBTYPE_SENDEVENT,          //!< VoipLocal() & VOIP_LOCAL_USER_SENDVOICE change
    VOIP_CBTYPE_TTOSEVENT,          //!< Text To Speech begin/end event
} VoipCbTypeE;

#if defined(DIRTYCODE_PC)
typedef enum VoipHeadsetDefaultDeviceTypeE
{
    VOIP_DEFAULTDEVICE_VOICECOM = 0,        //!< DRVM_MAPPER_CONSOLEVOICECOM_GET, no flags set
    VOIP_DEFAULTDEVICE_VOICECOM_ONLY,       //!< DRVM_MAPPER_CONSOLEVOICECOM_GET, flag set to DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY 
    VOIP_DEFAULTDEVICE_PREFERRED,           //!< DRVM_MAPPER_PREFERRED_GET, no flags set
    VOIP_DEFAULTDEVICE_PREFERRED_ONLY,      //!< DRVM_MAPPER_PREFERRED_GET, flag set to DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY 
} VoipHeadsetDefaultDeviceTypeE;
#endif

#if defined(DIRTYCODE_XBOXONE) || defined(DIRTYCODE_GDK)
#define VOIPXBOXONE_ID_MAXLEN                   (64)
#define VOIPXBOXONE_PERSONA_DISPLAYNAME_MAXLEN  (256)
#define VOIPXBOXONE_PERSONA_LANG_MAXLEN         (64)
#endif

//! config data to be used with VoipControl('voic') on PC, PS4, XboxOne (loopback) 
//! 'voic' is not supported on xone for voice chat via gamechat2 because that info is internally automatically retrieved from XBL.
typedef struct VoipSynthesizedSpeechCfgT
{
    int32_t iPersonaGender;     // zero=male, one=female
    #if defined(DIRTYCODE_PC)
    int32_t iLanguagePackCode;  // https://support.microsoft.com/en-us/help/324097/list-of-language-packs-and-their-codes-for-windows-2000-domain-control
    #endif
} VoipSynthesizedSpeechCfgT;

//! Metrics about transcribe events
typedef struct VoipSpeechToTextMetricsT
{
    uint32_t uEventCount;
    uint32_t uDurationMsSent;
    uint32_t uCharCountRecv;
    uint32_t uEmptyResultCount;
    uint32_t uErrorCount;
    uint32_t uDelay;
} VoipSpeechToTextMetricsT;

//! Metrics about narrate events
typedef struct VoipTextToSpeechMetricsT
{
    uint32_t uEventCount;
    uint32_t uCharCountSent;
    uint32_t uDurationMsRecv;
    uint32_t uEmptyResultCount;
    uint32_t uErrorCount;
    uint32_t uDelay;
} VoipTextToSpeechMetricsT;

/*** Macros ***************************************************************************************/

#define VOIP_ConnMask(_iConnID, _bEnable) ((_bEnable) << (_iConnID))

/*** Type Definitions *****************************************************************************/

//! The mode a given channel is used in
typedef enum VoipChanModeE
{
    VOIP_CHANNONE = 0, //<! (provided as a helper and so debuggers show the proper enum value.)
    VOIP_CHANSEND = 1, //<! Indicates a channel is used for sending
    VOIP_CHANRECV = 2, //<! Indicates a channel is used for receiving
    VOIP_CHANSENDRECV = (VOIP_CHANSEND | VOIP_CHANRECV) //<! (also has a helper)
} VoipChanModeE;

//! opaque module ref
typedef struct VoipRefT VoipRefT;

//! event callback function prototype
typedef void (VoipCallbackT)(VoipRefT *pVoip, VoipCbTypeE eCbType, int32_t iValue, void *pUserData);

/*! callback to be invoked when some transcribed text is ready to be displayed locally. (if iConnId is -1, originator is a local user, otherwise originator is a remote user)
    return TRUE if the text is displayed, FALSE if not */
typedef int32_t (VoipDisplayTranscribedTextCallbackT)(int32_t iConnId, int32_t iUserIndex, const char *pStrUtf8, void *pUserData);

//! speaker data callback (only supported on some platforms)
typedef void (VoipSpkrCallbackT)(int16_t *pFrameData, int32_t iNumSamples, void *pUserData);

//! callback to tell dirtysdk what the platform specific id is for this user
typedef uint64_t(VoipFirstPartyIdCallbackCbT)(uint64_t uPersonaId, void *pUserData);

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

//@}

#endif // _voipdef_h

