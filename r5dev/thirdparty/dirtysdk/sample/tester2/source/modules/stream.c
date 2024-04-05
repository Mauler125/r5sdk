/*H********************************************************************************/
/*!
    \File stream.c

    \Description
        Test the ProtoStream module.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 11/16/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/proto/protostream.h"
#include "DirtySDK/xml/xmlparse.h"

#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

#define STREAMER_USECALLBACK    (TRUE)

#define STREAMER_DEBUG          (FALSE)

#define STREAM_DEFAULTSKIP      (15)        // 15 seconds

#define STREAM_MAXURLS          (4)

/*** Type Definitions *************************************************************/

typedef struct StreamStationT
{
    char strName[32];
    char strInfo[64];
    char strType[32];
    char strUrl[STREAM_MAXURLS][256];
} StreamStationT;

typedef struct StreamPlaylistT
{
    int32_t iNumStations;
    StreamStationT Stations[1]; //!< variable-length array of stations
} StreamPlaylistT;

typedef struct StreamCmdRefT
{
    ProtoStreamRefT *pProtoStream;
    StreamPlaylistT *pPlaylist;
    int32_t         *pRandom;
    int32_t         iStartTick;
    int32_t         iSkipTick;
    int32_t         iSkipTime;
    int32_t         iCurStation;
    int32_t         iCurUrl;
    int32_t         iMetaInterval;
    int32_t         iMetaOffset;
    int32_t         iMetaSize;
    uint8_t         bRandomPlay;
    char            strMetaBuffer[(255*16)+1];
    char            strLastMetaBuffer[(255*16)+1];
} StreamCmdRefT;

/*** Function Prototypes **********************************************************/

static void _SubcmdStreamCreate(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SubcmdStreamDestroy(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SubcmdStreamOpen(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SubcmdStreamClose(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SubcmdStreamSkip(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);
static void _SubcmdStreamControl(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp);

/*** Variables ********************************************************************/

//! subcommand table
static T2SubCmdT _Stream_Commands[] =
{
    { "create",         _SubcmdStreamCreate   },
    { "destroy",        _SubcmdStreamDestroy  },
    { "open",           _SubcmdStreamOpen     },
    { "close",          _SubcmdStreamClose    },
    { "skip",           _SubcmdStreamSkip     },
    { "ctrl",           _SubcmdStreamControl  },
    { "",               NULL                  }
};

//! single instance of the stream module
static StreamCmdRefT *_Stream_pCmdRef = NULL;

//! basic playlist with sportscenter ref
static StreamPlaylistT _Playlist =
{
    1,
    {
        {
            "ESPN Sportscenter",
            "sportscenter 32k/sec",
            "Sports",
            {
                "http://stestbesl01.beta.ea.com/espnradio/sportscenter/sportscenter.mp3",
            }
        }
    },
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _StreamOpen

    \Description
        Open the stream specified by the given url

    \Input *pCmdRef     - module state
    \Input *pStation    - station reference
    \Input iRestartFreq - restart frequency
    
    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamOpen(StreamCmdRefT *pCmdRef, StreamStationT *pStation, int32_t iRestartFreq)
{
    char strTempUrl[128], *pUrl;

    // ref url
    pUrl = pStation->strUrl[pCmdRef->iCurUrl];

    // validate url    
    if (strncmp(pUrl, "http://", 7))
    {
        ZPrintf("stream: invalid URL '%s'\n", pUrl);
        return;
    }
    
    // see if URL needs a trailing slash
    if (!strchr(pUrl+7, '/'))
    {
        ds_strnzcpy(strTempUrl, pUrl, sizeof(strTempUrl));
        ds_strnzcat(strTempUrl, "/", sizeof(strTempUrl));
        pUrl = strTempUrl;
    }
    
    // open the stream
    if (pStation->strName[0] != '\0')
    {
        ZPrintf("stream: opening [%d] %s (url=%s)\n", (int32_t)(pStation - pCmdRef->pPlaylist->Stations), pStation->strName, pUrl);
    }
    else
    {
        ZPrintf("stream: opening url %s\n", pUrl);
    }
    ProtoStreamOpen(pCmdRef->pProtoStream, pUrl, iRestartFreq);
    pCmdRef->iMetaInterval = 0;
    pCmdRef->iMetaSize = 0;
    pCmdRef->iMetaOffset = 0;
}

/*F********************************************************************************/
/*!
    \Function _StreamXmlGetString

    \Description
        Get an xml entity's string contents

    \Input *pXml        - source xml to get data from
    \Input *pName       - name of entity
    \Input *pBuffer     - [out] storage for entity contents
    \Input iBufSize     - size of buffer pointed to by pBuffer
    
    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StreamXmlGetString(const char *pXml, const char *pName, char *pBuffer, int32_t iBufSize)
{
    ds_memclr(pBuffer, iBufSize);
    if ((pXml = XmlFind(pXml, pName)) != NULL)
    {
        return(XmlContentGetString(pXml, pBuffer, iBufSize, ""));
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _StreamDisplayPlaylist

    \Description
        Display the given playlist

    \Input *pCmdRef     - module state
    \Input *pPlaylist   - playlist to display
    
    \Output
        None.

    \Version 08/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamDisplayPlaylist(StreamCmdRefT *pCmdRef, StreamPlaylistT *pPlaylist)
{
    int32_t iStation;

    for (iStation = 0; iStation < pPlaylist->iNumStations; iStation++)
    {
        ZPrintf("stream: [%2d] %s\n", iStation, pPlaylist->Stations[iStation].strName);
    }
}

/*F********************************************************************************/
/*!
    \Function _StreamOpenPlaylist

    \Description
        Open the playlist specified by the given filename

    \Input *pCmdRef     - module state
    \Input *pName       - name of playlist to open
    
    \Output
        None.

    \Version 08/16/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamOpenPlaylist(StreamCmdRefT *pCmdRef, const char *pName)
{
    const char *pPlayFile, *pXml, *pXml2;
    int32_t iFileSize, iNumStations;

    // open the playlist
    ZPrintf("stream: opening playlist '%s'\n", pName);
    if ((pPlayFile = ZFileLoad(pName, &iFileSize, FALSE)) == NULL)
    {
        ZPrintf("stream: error opening playlist\n");
        return;
    }

    // see if it is a playlist
    if ((pXml = XmlFind(pPlayFile, "playlist.station")) == NULL)
    {
        ZPrintf("stream: file is not a playlist\n", pName);
        return;
    }

    // see how many stations are defined
    for (pXml2 = pXml, iNumStations = 1; ; )
    {
        if ((pXml2 = XmlSkip(pXml2)) == NULL)
        {
            break;
        }
        if (!strncmp(pXml2+1, "station", 7))
        {
            iNumStations += 1;
        }
    }
    
    ZPrintf("stream: parsed %d stations\n", iNumStations);
    if (iNumStations != 0)
    {
        int32_t iPlaylistSize, iRandom, iRandIdx;
        StreamStationT *pStation;
        int32_t *pTmpRandom;

        // allocate a new playlist structure
        if ((pCmdRef->pPlaylist != NULL) && (pCmdRef->pPlaylist != &_Playlist))
        {
            ZMemFree(pCmdRef->pPlaylist);
        }
        if (pCmdRef->pRandom != NULL)
        {
            ZMemFree(pCmdRef->pPlaylist);
        }
        iPlaylistSize = sizeof(*pCmdRef->pPlaylist) + (sizeof(pCmdRef->pPlaylist->Stations[0]) * (iNumStations - 1));
        pCmdRef->pPlaylist = ZMemAlloc(iPlaylistSize);
        ds_memclr(pCmdRef->pPlaylist, iPlaylistSize);
        pCmdRef->pPlaylist->iNumStations = iNumStations;

        // parse xml into new playlist
        for (pStation = pCmdRef->pPlaylist->Stations; ; )
        {
            if (!strncmp((char *)pXml+1, "station", 7))
            {
                _StreamXmlGetString(pXml, "station.name", pStation->strName, sizeof(pStation->strName));
                _StreamXmlGetString(pXml, "station.info", pStation->strInfo, sizeof(pStation->strInfo));
                _StreamXmlGetString(pXml, "station.type", pStation->strType, sizeof(pStation->strType));
                _StreamXmlGetString(pXml, "station.url0", pStation->strUrl[0], sizeof(pStation->strUrl[0]));
                _StreamXmlGetString(pXml, "station.url1", pStation->strUrl[1], sizeof(pStation->strUrl[1]));
                _StreamXmlGetString(pXml, "station.url2", pStation->strUrl[2], sizeof(pStation->strUrl[2]));
                _StreamXmlGetString(pXml, "station.url3", pStation->strUrl[3], sizeof(pStation->strUrl[3]));
                pStation += 1;
            }
            if ((pXml = XmlSkip(pXml)) == NULL)
            {
                break;
            }
        }
        
        // create new buffer for randomizing playlist
        pCmdRef->pRandom = ZMemAlloc(iNumStations*sizeof(*pCmdRef->pRandom));

        // create playlist indices
        pTmpRandom = ZMemAlloc((iNumStations+1)*sizeof(*pCmdRef->pRandom));
        for (iRandom = 0; iRandom < iNumStations; iRandom++)
        {
            pTmpRandom[iRandom] = iRandom;
        }

        // generate random walk through playlist
        for (iRandom = 0; iRandom < (iNumStations-1); iRandom++)
        {
            iRandIdx = rand() % (iNumStations-iRandom);
            pCmdRef->pRandom[iRandom] = pTmpRandom[iRandIdx];
            memmove(&pTmpRandom[iRandIdx], &pTmpRandom[iRandIdx+1], (iNumStations-iRandIdx-1)*sizeof(*pTmpRandom));
        }
        pCmdRef->pRandom[iRandom] = pTmpRandom[0];

        // free temp buffer
        ZMemFree(pTmpRandom);
    }

    // done with the file    
    ZMemFree((void *)pPlayFile);
}

/*F********************************************************************************/
/*!
    \Function _StreamGetRandIndex

    \Description
        Get index of given station in random playlist.

    \Input *pCmdRef     - module state
    \Input iIndex       - station index in playlist
    
    \Output
        int32_t         - index of station in random playlist

    \Version 11/14/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StreamGetRandIndex(StreamCmdRefT *pCmdRef, int32_t iIndex)
{
    int32_t iStation;
    for (iStation = 0; iStation < pCmdRef->pPlaylist->iNumStations; iStation++)
    {
        if (pCmdRef->pRandom[iStation] == iIndex)
        {
            return(iStation);
        }
    }
    NetPrintf(("stream: unable to find station %d in random playlist\n", iIndex));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _StreamClose

    \Description
        Close the current stream, if any

    \Input *pCmdRef     - module state
    
    \Output
        None.

    \Version 11/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamClose(StreamCmdRefT *pCmdRef)
{
    // close the stream
    ZPrintf("stream: closing active stream\n");
    ProtoStreamClose(pCmdRef->pProtoStream);
}

/*F********************************************************************************/
/*!
    \Function _StreamGetCurStation

    \Description
        Get index of current station (possibly applying random index)

    \Input *pCmdRef     - module state
    
    \Output
        int32_t         - index of current station

    \Version 11/24/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StreamGetCurStation(StreamCmdRefT *pCmdRef)
{
    int32_t iStation;
    iStation = (pCmdRef->bRandomPlay) ? pCmdRef->pRandom[pCmdRef->iCurStation] : pCmdRef->iCurStation;
    return(iStation);
}

/*F********************************************************************************/
/*!
    \Function _StreamNextUrl

    \Description
        Skip to next url for current station

    \Input *pCmdRef     - module state
    
    \Output
        None.

    \Version 11/22/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamNext(StreamCmdRefT *pCmdRef)
{
    pCmdRef->iCurUrl = pCmdRef->iCurUrl + 1;
    _StreamOpen(pCmdRef, &pCmdRef->pPlaylist->Stations[_StreamGetCurStation(pCmdRef)], PROTOSTREAM_FREQ_IMMED);
}

/*F********************************************************************************/
/*!
    \Function _StreamSkip

    \Description
        Skip to next stream.

    \Input *pCmdRef     - module state
    \Input iCurTick     - current tick
    
    \Output
        None.

    \Version 11/22/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamSkip(StreamCmdRefT *pCmdRef, int32_t iCurTick)
{
    // switch to next station
    pCmdRef->iCurStation = (pCmdRef->iCurStation + 1) % pCmdRef->pPlaylist->iNumStations;

    // switch to next stream
    pCmdRef->iCurUrl = 0;

    // open the stream
    _StreamOpen(pCmdRef, &pCmdRef->pPlaylist->Stations[_StreamGetCurStation(pCmdRef)], PROTOSTREAM_FREQ_IMMED);

    // set next update
    pCmdRef->iSkipTick = iCurTick;
}

/*F********************************************************************************/
/*!
    \Function _StreamParseHeader

    \Description
        Parse data from from icy header response

    \Input *pCmdRef - module state
    \Input *pHeader - http response header
    
    \Output
        None.

    \Version 03/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamParseHeader(StreamCmdRefT *pCmdRef, const char *pHeader)
{
    char strData[256], *pData, *pHeaderInfo;

    // parse meta-interval data, if present
    if ((pHeaderInfo = ds_stristr(pHeader, "icy-metaint")) != NULL)
    {
        // parse meta interval
        while (!isdigit(*pHeaderInfo))
        {
            pHeaderInfo += 1;
        }
        pCmdRef->iMetaInterval = strtol(pHeaderInfo, NULL, 10);
        NetPrintf(("stream: metadata interval is %d\n", pCmdRef->iMetaInterval));

        pCmdRef->iMetaOffset = pCmdRef->iMetaInterval;
    }
    
    // parse icy-name, if present
    if ((pHeaderInfo = ds_stristr(pHeader, "icy-name:")) != NULL)
    {
        pHeaderInfo += sizeof("icy-name:")-1;
        for (pData = strData; *pHeaderInfo != '\r'; pHeaderInfo += 1, pData += 1)
        {
            *pData = *pHeaderInfo;
        }
        *pData = '\0';
        ZPrintf("=====================================================================================================\n");
        ZPrintf("stream: %s\n", strData);
    }

    // parse icy-genre, if present
    if ((pHeaderInfo = ds_stristr(pHeader, "icy-genre:")) != NULL)
    {
        pHeaderInfo += sizeof("icy-genre:")-1;
        for (pData = strData; *pHeaderInfo != '\r'; pHeaderInfo += 1, pData += 1)
        {
            *pData = *pHeaderInfo;
        }
        *pData = '\0';
        ZPrintf("stream: %s\n", strData);
    }

    // parse icy-br, if present
    if ((pHeaderInfo = ds_stristr(pHeader, "icy-br:")) != NULL)
    {
        pHeaderInfo += sizeof("icy-br:")-1;
        for (pData = strData; *pHeaderInfo != '\r'; pHeaderInfo += 1, pData += 1)
        {
            *pData = *pHeaderInfo;
        }
        *pData = '\0';
        ZPrintf("stream: %skbps\n", strData);
        ZPrintf("=====================================================================================================\n");
    }
}

/*F********************************************************************************/
/*!
    \Function _StreamProcessMetaData

    \Description
        Process metadata embedded in mp3 stream.

    \Input *pCmdRef - module state
    \Input *pData   - stream data
    \Input iSize    - amount of data available for processing
    
    \Output
        int32_t     - number of bytes consumed

    \Version 03/23/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StreamProcessMetaData(StreamCmdRefT *pCmdRef, const uint8_t *pData, int32_t iSize)
{
    int32_t iConsume, iConsumed;

    // process all data
    for ( iConsumed = 0; iSize > 0; )
    {
        // if we're processing mp3 data, bail
        if (pCmdRef->iMetaOffset > 0)
        {
            break;
        }

        // are we processing a metadata header?
        if (pCmdRef->iMetaSize > 0)
        {
            if (iSize >= pCmdRef->iMetaSize)
            {
                iConsume = pCmdRef->iMetaSize;
                pCmdRef->iMetaOffset = pCmdRef->iMetaInterval;
            }
            else
            {
                iConsume = iSize;
            }

            // copy metadata into buffer
            ds_strsubzcat(pCmdRef->strMetaBuffer, sizeof(pCmdRef->strMetaBuffer), (char *)pData, iConsume);

            // skip metadata
            pData += iConsume;
            iSize -= iConsume;

            // mark metasize as consumed
            pCmdRef->iMetaSize -= iConsume;
            iConsumed += iConsume;
        }
        // do we have a metadata header?
        else if (pCmdRef->iMetaOffset == 0)
        {
            pCmdRef->iMetaSize = *pData * 16;
            pCmdRef->strMetaBuffer[0] = '\0';
            pData += 1;
            iSize -= 1;
            iConsumed += 1;
        }

        // if we're done with the header, reset the offset
        if (pCmdRef->iMetaSize == 0)
        {
            // did we get any metadata?
            if (pCmdRef->strMetaBuffer[0] != '\0')
            {
                // did the metadata change since the last update?
                if (strcmp(pCmdRef->strLastMetaBuffer, pCmdRef->strMetaBuffer))
                {
                    char *pTitle, *pEndTitle;

                    // first, save the update
                    strcpy(pCmdRef->strLastMetaBuffer, pCmdRef->strMetaBuffer);

                    // try and find title for display
                    if ((pTitle = strstr(pCmdRef->strMetaBuffer, "StreamTitle")) != NULL)
                    {
                        if ((pTitle = strchr(pTitle, '\'')) != NULL)
                        {
                            time_t uTime;
                            
                            pTitle += 1;
                            if ((pEndTitle = strchr(pTitle, ';')) != NULL)
                            {
                                *(pEndTitle-1) = '\0';
                            }
                            else if ((pEndTitle = strchr(pTitle, '\'')) != NULL)
                            {
                                *pEndTitle = '\0';
                            }

                            if ((uTime = ds_timeinsecs()) != 0)
                            {
                                struct tm CurTime;
                                ds_secstotime(&CurTime, uTime);
                                ZPrintf("stream: %02d:%02d now playing - %s\n", CurTime.tm_hour, CurTime.tm_min, pTitle);
                            }
                            else
                            {
                                ZPrintf("stream: now playing - %s\n", pTitle);
                            }
                        }
                    }
                }
            }

            // reset offset
            pCmdRef->iMetaOffset = pCmdRef->iMetaInterval;
        }
    }

    return(iConsumed);
}

/*F********************************************************************************/
/*!
    \Function _StreamDone

    \Description
        Process stream completion

    \Input *pCmdRef     - module state
    \Input iCurTick     - current tick
    
    \Output
        None.

    \Version 03/31/2008 (jbrookes)
*/
/********************************************************************************F*/
static void _StreamDone(StreamCmdRefT *pCmdRef, int32_t iCurTick)
{
    if (pCmdRef->pPlaylist->iNumStations > 1)
    {
        int32_t iNextUrl = pCmdRef->iCurUrl + 1;
        if ((iNextUrl < STREAM_MAXURLS) && (pCmdRef->pPlaylist->Stations[_StreamGetCurStation(pCmdRef)].strUrl[iNextUrl][0] != '\0'))
        {
            _StreamNext(pCmdRef);
        }
        else
        {
            _StreamSkip(pCmdRef, iCurTick);
        }
    }
    else
    {
        // if it's a one-shot, just stop playback
        ProtoStreamClose(pCmdRef->pProtoStream);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoStreamCallback

    \Description
        ProtoStream callback handler

    \Input *pProtoStream    - protostream module state
    \Input eStatus          - callback status
    \Input *pBuffer         - pointer to buffered data, or NULL if no data
    \Input iBufSize         - amount of data available in buffer
    \Input *pUserData       - user data pointer    

    \Output
        int32_t             - number of bytes consumed

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
#if STREAMER_USECALLBACK
static int32_t _ProtoStreamCallback(ProtoStreamRefT *pProtoStream, ProtoStreamStatusE eStatus, const uint8_t *pBuffer, int32_t iBufSize, void *pUserData)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)pUserData;
    const uint8_t *pBufStart = pBuffer;
    int32_t iResult;
    #if STREAMER_DEBUG
    int32_t iTick;
    #endif

    // validate cmdref
    if ((pCmdRef == NULL) || (pCmdRef->pProtoStream != pProtoStream))
    {
        NetPrintf(("stream: error -- invalid callback ref\n"));
        return(0);
    }

    // handle stream start state
    if (eStatus == PROTOSTREAM_STATUS_BEGIN)
    {
        char strHeader[512];
        
        // get header
        ProtoStreamStatus(pProtoStream, 'htxt', strHeader, sizeof(strHeader));
        
        // print header
        NetPrintf(("stream: stream start\n"));

        // parse icy header
        _StreamParseHeader(pCmdRef, strHeader);

        // save start time
        pCmdRef->iStartTick = ZTick();
    }

    // calc current tick
    #if STREAMER_DEBUG
    iTick = ZTick() - pCmdRef->iStartTick;
    #endif
    
    // handle stream data ready state
    if ((eStatus == PROTOSTREAM_STATUS_BEGIN) || (eStatus == PROTOSTREAM_STATUS_DATA))
    {
        // decode processing
        for ( iResult = 1; iResult > 0; )
        {
            // process metadata, if any
            if (pCmdRef->iMetaInterval != 0)
            {
                // clamp maximum amount of data
                iResult = _StreamProcessMetaData(pCmdRef, pBuffer, iBufSize);
                if (iResult > 0)
                {
                    pBuffer += iResult;
                    iBufSize -= iResult;
                }
                // make sure we don't consume data into the next metaheader
                if (iBufSize > pCmdRef->iMetaOffset)
                {
                    iBufSize = pCmdRef->iMetaOffset;
                }
            }

            // process mp3 data
            if (iBufSize > 0)
            {
                // $$TODO$$ mimic mp3 decoding: for now just consume the entire buffer
                pBuffer += iBufSize;

                // update meta offset
                if (pCmdRef->iMetaInterval != 0)
                {
                    pCmdRef->iMetaOffset -= iBufSize;
                }
            }
            else
            {
                break;
            }
        }
    }

    // handle stream done state
    if (eStatus == PROTOSTREAM_STATUS_DONE)
    {
        // kill the player
        NetPrintf(("stream: stream done; resetting player\n"));
        _StreamDone(pCmdRef, ZTick());
    }

    #if STREAMER_DEBUG
    if ((pBuffer - pBufStart) > 0)
    {
        // display status info
        ZPrintf("stream: consumed %d bytes at %d:%02d:%03d\n", pBuffer - pBufStart,
            iTick / (1000*60),          // minutes
            (iTick / 1000) % 60,        // seconds
            iTick % 1000);              // thousandths of a second
    }
    #endif

    return(pBuffer - pBufStart);
}
#endif

/*F********************************************************************************/
/*!
    \Function _StreamUpdate

    \Description
        Read data from ProtoStream (non-callback interface)

    \Input *pProtoStream    - protostream module state
    \Input eStatus          - callback status
    \Input *pBuffer         - pointer to buffered data, or NULL if no data
    \Input iBufSize         - amount of data available in buffer
    \Input *pUserData       - user data pointer    

    \Output
        int32_t             - number of bytes consumed

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
#if !STREAMER_USECALLBACK
static int32_t _StreamUpdate(StreamCmdRefT *pCmdRef)
{
    static char aBuffer[64 * 1024];
    int32_t iResult;

    if ((iResult = ProtoStreamRead(pCmdRef->pProtoStream, aBuffer, sizeof(aBuffer), 64)) > 0)
    {

    }

    return(iResult);
}
#endif


/*
   Stream Commands
*/

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamCreate
    
    \Description
        Stream subcommand - create the module
    
    \Input *_pCmdRef    - unused
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamCreate(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef;
    int32_t iBufSize;
    
    // usage
    if ((argc > 3) || (bHelp == TRUE))
    {
        ZPrintf("   usage: %s create [bufsize]\n", argv[0]);
        return;
    }

    // get buffer size
    iBufSize = (argc == 3) ? strtol(argv[2], NULL, 10) * 1024 : 32 * 1024;

    // allocate context
    pCmdRef = _Stream_pCmdRef = ZMemAlloc(sizeof(*pCmdRef));
    ds_memclr(pCmdRef, sizeof(*pCmdRef));

    // create the streaming module
    pCmdRef->pProtoStream = ProtoStreamCreate(iBufSize);

#if STREAMER_USECALLBACK
    // set up callback info
    ProtoStreamSetCallback(pCmdRef->pProtoStream, 100, _ProtoStreamCallback, pCmdRef);
#endif

    // allow shoutcast server compatibility (shoutcast returns "ICY" instead of "HTTP" response)
    ProtoStreamControl(pCmdRef->pProtoStream, 'hver', FALSE, 0, NULL);

    // pretend to be WinAmp and enable metadata, as some servers require these
    ProtoStreamControl(pCmdRef->pProtoStream, 'apnd', 0, 0,
        "User-Agent: WinampMPEG/5.11\r\n"
        "Icy-MetaData:1\r\n"
        "Accept: */*\r\n");

    // set default playlist
    pCmdRef->pPlaylist = &_Playlist;

    // seed random number generator for random playlist traversal
    srand(NetTick());
    // enable random play
    pCmdRef->bRandomPlay = TRUE;
}

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamDestroy
    
    \Description
        Stream subcommand - destroy module
    
    \Input *_pCmdRef    - module state
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamDestroy(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)_pCmdRef;

    // validate arguments
    if ((argc != 2) || bHelp)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        return;
    }

    // close current strean, if any
    _StreamClose(pCmdRef);

    // free playlist, if any
    if (pCmdRef->pPlaylist != &_Playlist)
    {
        ZMemFree(pCmdRef->pPlaylist);
    }
    if (pCmdRef->pRandom != NULL)
    {
        ZMemFree(pCmdRef->pRandom);
    }

    // destroy streamer
    ProtoStreamDestroy(pCmdRef->pProtoStream);

    // destroy module state
    ZMemFree(pCmdRef);
    _Stream_pCmdRef = NULL;
}

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamOpen
    
    \Description
        Stream subcommand - open a stream
    
    \Input *_pCmdRef    - module state
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamOpen(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)_pCmdRef;
    StreamStationT Station, *pStation;
    int32_t iFreq, iStation = -1;

    // validate arguments
    if (((argc != 3) && (argc != 4)) || bHelp)
    {
        ZPrintf("   usage: %s open [url|playlist|index] [freq]\n", argv[0]);
        return;
    }

    // playlist?
    if (strstr(argv[2], ".xml"))
    {
        _StreamOpenPlaylist(pCmdRef, argv[2]);
        _StreamDisplayPlaylist(pCmdRef, pCmdRef->pPlaylist);
        return;
    }
    else if (!ds_strnicmp(argv[2], "http", 4)) // built-in url?
    {
        ds_memclr(&Station, sizeof(Station));
        ds_strnzcpy(Station.strUrl[0], argv[2], sizeof(Station.strUrl[0]));
        pCmdRef->iCurUrl = 0;
        pStation = &Station;
    }
    else // assume it is a playlist index
    {
        if ((iStation = strtol(argv[2], NULL, 10)) >= pCmdRef->pPlaylist->iNumStations)
        {
            iStation = 0;
        }
        pStation = &pCmdRef->pPlaylist->Stations[iStation];
        pCmdRef->iCurStation = pCmdRef->bRandomPlay ? _StreamGetRandIndex(pCmdRef, iStation) : iStation;
    }

    // get restart frequency
    iFreq = (argc == 4) ? strtol(argv[3], NULL, 10) : PROTOSTREAM_FREQ_IMMED;

    // reset url index
    pCmdRef->iCurUrl = 0;
    
    // open the stream
    _StreamOpen(pCmdRef, pStation, iFreq);
}

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamClose
    
    \Description
        Stream subcommand - close a stream
    
    \Input *_pCmdRef    - module state
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamClose(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)_pCmdRef;

    // validate arguments
    if ((argc != 2) || bHelp)
    {
        ZPrintf("   usage: %s close\n", argv[0]);
        return;
    }

    // reset skip
    pCmdRef->iSkipTime = 0;

    // destroy stream
    _StreamClose(pCmdRef);
}

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamSkip
    
    \Description
        Stream subcommand - enable skip mode
    
    \Input *_pCmdRef    - module state
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamSkip(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)_pCmdRef;

    // validate arguments
    if (((argc != 2) && (argc != 3)) || bHelp)
    {
        ZPrintf("   usage: %s skip [random]|[time]\n", argv[0]);
        return;
    }

    // activate?
    if ((pCmdRef->iSkipTime == 0) || (argc == 3))
    {
        // get skip time in seconds
        pCmdRef->iSkipTime = (argc == 3) ? strtol(argv[2], NULL, 10) : STREAM_DEFAULTSKIP;
        
        // convert to milliseconds
        pCmdRef->iSkipTime *= 1000;
    }
    else
    {
        // stop skipping
        pCmdRef->iSkipTime = 0;
    }
}

/*F*************************************************************************************/
/*!
    \Function _SubcmdStreamControl
    
    \Description
        Stream subcommand - call control function
    
    \Input *_pCmdRef    - module state
    \Input argc         - argument count
    \Input *argv[]      - argument list
    \Input bHelp        - TRUE means display command help
    
    \Output None.
            
    \Version 1.0 11/21/2005 (jbrookes) First Version
*/
/**************************************************************************************F*/
static void _SubcmdStreamControl(void *_pCmdRef, int32_t argc, char *argv[], unsigned char bHelp)
{
    StreamCmdRefT *pCmdRef = (StreamCmdRefT *)_pCmdRef;
    int32_t iCmd, iValue=0, iValue2=0;
    void *pValue=NULL;

    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s ctrl <args>\n", argv[0]);
        return;
    }

    iCmd  = argv[2][0] << 24;
    iCmd |= argv[2][1] << 16;
    iCmd |= argv[2][2] << 8;
    iCmd |= argv[2][3];

    if (argc > 3)
    {
        iValue = strtol(argv[3], NULL, 10);
    }
    
    if (argc > 4)
    {
        iValue2 = strtol(argv[4], NULL, 10);
    }

    // handle stream-specific commands
    if (iCmd == 'rand')
    {
        pCmdRef->bRandomPlay = !pCmdRef->bRandomPlay;
        ZPrintf("stream: random play %s\n", pCmdRef->bRandomPlay ? "enabled" : "disabled");
        return;
    }
    // pass unhandled selectors to ProtoStreamControl();    
    ZPrintf("stream: executing ProtoStreamControl(pProtoUpnp, '%s', %d, %d, %s)\n", argv[2], iValue, iValue2, pValue ? pValue : "(null)");
    ProtoStreamControl(pCmdRef->pProtoStream, iCmd, iValue, iValue2, pValue);
}

/*F********************************************************************************/
/*!
    \Function _CmdStreamCb

    \Description
        Recurring stream callback.

    \Input *argz    - environment
    \Input argc     - number of args
    \Input *argv[]  - argument list
    
    \Output int32_t - standard return code

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdStreamCb(ZContext *argz, int32_t argc, char *argv[])
{
    StreamCmdRefT *pCmdRef = _Stream_pCmdRef;
    int32_t iCurTick = ZTick();

    // if no ref, we're done
    if (pCmdRef == NULL)
    {
        return(0);
    }

    // check for kill
    if (argc == 0)
    {
        char *strArgs[2] = { "stream", "destroy" };
        ZPrintf("%s: killed\n", argv[0]);
        _SubcmdStreamDestroy(pCmdRef, 2, strArgs, 0);
        return(0);
    }

    // update skip processing
    if (pCmdRef->iSkipTime != 0)
    {
        if ((iCurTick - pCmdRef->iSkipTick) > pCmdRef->iSkipTime)
        {
            _StreamSkip(pCmdRef, iCurTick);
        }
    }

    // update protostream module
    ProtoStreamUpdate(pCmdRef->pProtoStream);

    #if !STREAMER_USECALLBACK
    _StreamUpdate(pCmdRef);
    #endif

    // keep running
    return(ZCallback(&_CmdStreamCb, 16));
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdStream

    \Description
        Create the Module module.

    \Input *argz    - environment
    \Input argc     - number of args
    \Input *argv[]  - argument list
    
    \Output int32_t - standard return code

    \Version 11/16/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdStream(ZContext *argz, int32_t argc, char *argv[])
{
    unsigned char bHelp, bCreate = FALSE;
    StreamCmdRefT *pCmdRef = _Stream_pCmdRef;
    T2SubCmdT *pCmd;

    // handle basic help
    if ((argc < 2) || (((pCmd = T2SubCmdParse(_Stream_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   test the protostream module\n");
        T2SubCmdUsage(argv[0], _Stream_Commands);
        return(0);
    }

    // if no ref yet, make one
    if ((pCmdRef == NULL) && strcmp(pCmd->strName, "create"))
    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _SubcmdStreamCreate(pCmdRef, 1, &pCreate, bHelp);
        pCmdRef = _Stream_pCmdRef;
        bCreate = TRUE;
    }

    // hand off to command
    pCmd->pFunc(pCmdRef, argc, argv, bHelp);

    // if we executed create, remember
    if (pCmd->pFunc == _SubcmdStreamCreate)
    {
        bCreate = TRUE;
    }

    // if we executed create, install periodic callback
    return((bCreate == TRUE) ? ZCallback(_CmdStreamCb, 100) : 0);
}
