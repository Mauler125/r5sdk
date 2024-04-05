/*H********************************************************************************/
/*!
    \File protostream.h

    \Description
        Manage streaming of an Internet media resource.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _protostream_h
#define _protostream_h

/*!
\Moduledef ProtoStream ProtoStream
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttp.h"

/*** Defines **********************************************************************/

/* when a stream is opened, the repeat frequency is specified in seconds. these
   constants handle special cases of "one time play" (once) and "immediate
   restart" (immed). */
#define PROTOSTREAM_FREQ_ONCE   (-1)    //!< one time play
#define PROTOSTREAM_FREQ_IMMED  (0)     //!< immediate restart

//! callback status indicating state of the stream
typedef enum ProtoStreamStatusE
{
    PROTOSTREAM_STATUS_BEGIN = 0,   //!< first buffer of data (start of stream)
    PROTOSTREAM_STATUS_DATA,        //!< data has been received
    PROTOSTREAM_STATUS_SYNC,        //!< data was dropped from the stream
    PROTOSTREAM_STATUS_DONE         //!< end of stream (no data)
} ProtoStreamStatusE;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct ProtoStreamRefT ProtoStreamRefT;

//! data callback definition
typedef int32_t (ProtoStreamCallbackT)(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus, const uint8_t *pBuffer, int32_t iBufSize, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
DIRTYCODE_API ProtoStreamRefT *ProtoStreamCreate(int32_t iBufSize);

// destroy the module
DIRTYCODE_API void ProtoStreamDestroy(ProtoStreamRefT *pProtoStream);

// set recurring callback
DIRTYCODE_API void ProtoStreamSetCallback(ProtoStreamRefT *pProtoStream, int32_t iRate, ProtoStreamCallbackT *pCallback, void *pUserData);

// set http custom header send/recv callbacks
DIRTYCODE_API void ProtoStreamSetHttpCallback(ProtoStreamRefT *pProtoStream, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserData);

// begin streaming an internet media resource
DIRTYCODE_API int32_t ProtoStreamOpen(ProtoStreamRefT *pProtoStream, const char *pUrl, int32_t iFreq);

// begin streaming an internet media resource, with a request body  (using POST)
DIRTYCODE_API int32_t ProtoStreamOpen2(ProtoStreamRefT *pProtoStream, const char *pUrl, const char *pReq, int32_t iFreq);

// read at least iMinLen bytes from stream into user buffer
DIRTYCODE_API int32_t ProtoStreamRead(ProtoStreamRefT *pProtoStream, char *pBuffer, int32_t iBufLen, int32_t iMinLen);

// pause stream
DIRTYCODE_API void ProtoStreamPause(ProtoStreamRefT *pProtoStream, uint8_t bPause);

// stop streaming
DIRTYCODE_API void ProtoStreamClose(ProtoStreamRefT *pProtoStream);

// get module status
DIRTYCODE_API int32_t ProtoStreamStatus(ProtoStreamRefT *pProtoStream, int32_t iStatus, void *pBuffer, int32_t iBufSize);

// set control options
DIRTYCODE_API int32_t ProtoStreamControl(ProtoStreamRefT *pProtoStream, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// update module
DIRTYCODE_API void ProtoStreamUpdate(ProtoStreamRefT *pProtoStream);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protostream_h

