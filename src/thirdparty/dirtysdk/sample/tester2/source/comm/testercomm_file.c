/*H********************************************************************************/
/*!
    \File testercomm_file.c

    \Description
        This module provides a communication layer between the host and the client.  
        Typical operations are SendLine() and GetLine(), which send and receive 
        lines of text, commands, debug output, etc.  Each platform will implement 
        its own way of communicating  through files, debugger API calls, etc.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/23/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zmem.h"
#include "libsample/zlib.h"
#include "libsample/zlist.h"
#include "libsample/zfile.h"
#include "testerprofile.h"
#include "testerregistry.h"
#include "testercomm.h"

/*** Defines **********************************************************************/

#define TESTERCOMM_FILEPATHSIZE_DEFAULT     (256)       //!< path/file storage
#define TESTERCOMM_PATH_DEFAULT             (TESTERPROFILE_CONTROLDIR_VALUEDEFAULT)  //!< default path for control files

#define TESTERCOMM_OUTPUTTIMEOUT_DEFAULT    (60*1000)   //!< timeout (ms) for a output message

#define TESTERCOMM_FILEBUFFERSIZE           (4096)      //!< size of temp file buffer to use

#define TESTERCOMM_FILEPROFILING            (0)         //! turn this on to see times for file writing

/*** Type Definitions *************************************************************/

typedef struct TesterCommFileT TesterCommFileT;
struct TesterCommFileT
{
    // file specific stuff
    char strControlDir[TESTERPROFILE_CONTROLDIR_SIZEDEFAULT];           //!< location of control directory
    char strTempPathFile[TESTERCOMM_FILEPATHSIZE_DEFAULT];    //!< temporary output filename
    char strInputPathFile [TESTERCOMM_FILEPATHSIZE_DEFAULT];  //!< input file filename
    char strOutputPathFile[TESTERCOMM_FILEPATHSIZE_DEFAULT];  //!< output file filename
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _TesterCommCheckInput

    \Description
        Check for data coming from the other side (host or client) and pull
        any data into our internal buffer, if possible.

    \Input *pState - pointer to host client comm module

    \Output int32_t - 0 = no data, >0 = data, error code otherwise

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckInput(TesterCommT *pState)
{
    TesterCommFileT *pInterfaceData;
    char *pInputData;
    const char *pInputLoop;
    int32_t iInputDataSize;
    ZFileT iInputFile;
    uint16_t aJson[16*1024];

    if ((pState == NULL) || (pState->pInterface->pData == NULL))
    {
        return(-1);
    }
    pInterfaceData = pState->pInterface->pData;

    // check to make sure our filename is sane
    if (strlen(pInterfaceData->strInputPathFile) == 0)
    {
        return(-1);
    }

    // open the input file first
    iInputFile = ZFileOpen(pInterfaceData->strInputPathFile, ZFILE_OPENFLAG_RDONLY | ZFILE_OPENFLAG_APPEND);

    if (iInputFile == ZFILE_INVALID)
    {
        // could not open it - nothing there - not necessarily an error
        return(-1);
    }

    // get the file size
    iInputDataSize = (int32_t)ZFileSize(iInputFile);

    // close the file
    ZFileClose(iInputFile);

    // check for empty file
    if (iInputDataSize == 0)
    {
        // delete the file
        ZFileDelete(pInterfaceData->strInputPathFile);
        // and quit
        return(0);
    }

    // load the contents of the file into the buffer
    pInputData = ZFileLoad(pInterfaceData->strInputPathFile, &(iInputDataSize), 0);
    JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), pInputData, iInputDataSize);

    // and delete the file
    ZFileDelete(pInterfaceData->strInputPathFile);

    if (pInputData == NULL)
    {
        // no data to process
        return(0);
    }

    // remember that we got input
    pState->bGotInput = TRUE;

    // push each line onto the list
    iInputDataSize = 0;
    while ((pInputLoop = JsonFind2(aJson, NULL, "msg[", iInputDataSize)) != NULL)
    {
        char strBuffer[sizeof(pState->LineData.strBuffer)];
        int32_t iBufLen;

        ds_memclr(&pState->LineData, sizeof(pState->LineData));
        pState->LineData.iType = (int32_t)JsonGetInteger(JsonFind2(aJson, pInputLoop, ".TYPE", iInputDataSize), 0);
        iBufLen = JsonGetString(JsonFind2(aJson, pInputLoop, ".TEXT", iInputDataSize), strBuffer, sizeof(strBuffer), "");
        Base64Decode3(strBuffer, iBufLen, pState->LineData.strBuffer, sizeof(pState->LineData.strBuffer));
        // add to the back of the list
        // remember to add one for the terminator
        ZListPushBack(pState->pInputData, &pState->LineData);

        iInputDataSize += 1;
    }

    // kill the file memory we used to store the temp input
    ZMemFree(pInputData);

    // done - return how much we got from the file
    return(iInputDataSize);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommCheckOutput

    \Description
        Check and send data from the output buffer, if possible.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/24/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommCheckOutput(TesterCommT *pState)
{
    TesterCommFileT *pInterfaceData;
    ZFileStatT FileStat;
    ZFileT iTempFile;
    int32_t iResult;
    int32_t iLineLength=0;
#if TESTERCOMM_FILEPROFILING
    int32_t iTime1, iTime2;
#endif

    // file writing stuff
    const char *pFileBuf;

    // check for error conditions
    if ((pState == NULL) || (pState->pInterface->pData == NULL))
    {
        return(-1);
    }
    pInterfaceData = pState->pInterface->pData;

    // see if there's any data to send
    if (ZListPeekFront(pState->pOutputData) == NULL)
    {
        // reset the last send time - we're not timing out because there's no data
        pState->uLastSendTime = ZTick();
        return(0);
    }

    // see if we've timed out
    if (ZTick() - pState->uLastSendTime > TESTERCOMM_OUTPUTTIMEOUT_DEFAULT)
    {
        // timeout occurred - dump all pending messages
        // and pop a warning
        ZListClear(pState->pOutputData);
        ZPrintf("testercomm: TIMEOUT in sending data.  List cleared.  Comms Lost?\n");
    }

    // we've got data to send in the output list
    // see if we can send it in the file
    iResult = ZFileStat(pInterfaceData->strOutputPathFile, &FileStat);
    
    // only do something if the file doesn't exist
    // so quit if the NOSUCHFILE isn't set
    if (iResult != ZFILE_ERROR_NOSUCHFILE)
        return(0);

    // open the file
    iTempFile = ZFileOpen(pInterfaceData->strTempPathFile, ZFILE_OPENFLAG_WRONLY | ZFILE_OPENFLAG_CREATE);
    if (iTempFile == ZFILE_INVALID)
    {
        // could not open it - error condition
        return(-1);
    }

#if TESTERCOMM_FILEPROFILING
    printf("--**-- [%12d] Starting get from buffer to file \n", ZTick());
#endif
    JsonInit(pState->strCommand, sizeof(pState->strCommand), 0);
    JsonArrayStart(pState->strCommand, "msg");
    while(ZListPeekFront(pState->pOutputData))
    {
        char strBuffer[sizeof(pState->LineData.strBuffer)];

        // snag the data into the local buffer
        ZListPopFront(pState->pOutputData, &pState->LineData);

        // create the stuff to write
        JsonObjectStart(pState->strCommand, NULL);
        JsonAddInt(pState->strCommand, "TYPE", pState->LineData.iType);
        Base64Encode2(pState->LineData.strBuffer, (int32_t)strlen(pState->LineData.strBuffer), strBuffer, sizeof(strBuffer));
        if ((iResult = JsonAddStr(pState->strCommand, "TEXT", strBuffer)) == JSON_ERR_FULL)
        {
            // Sometimes, the text we get in will be too big.  Make sure this is an easy error to diagnose.
            printf("Text too large to send in TESTERCOMM_COMMANDSIZE_DEFAULT.  Discarding.\n");
        }
        JsonObjectEnd(pState->strCommand);
    }
    JsonArrayEnd(pState->strCommand);
    pFileBuf = JsonFinish(pState->strCommand);
    iLineLength = (int32_t)strlen(pFileBuf);

    // write the last little chunk
    if (iLineLength > 0)
    {
#if TESTERCOMM_FILEPROFILING
        iTime1 = ZTick();
#endif
        ZFileWrite(iTempFile, (void *)pFileBuf, iLineLength);
#if TESTERCOMM_FILEPROFILING
        iTime2 = ZTick();
        printf("------ [%12d] Writing [%d] bytes to file took [%d] Ticks.\n", 
            ZTick(), iWrittenSize, iTime2 - iTime1);
#endif
    }

#if TESTERCOMM_FILEPROFILING
    printf("--**-- [%12d] Done pushing from buffer to file \n", ZTick());
#endif

    // close the temp file
    ZFileClose(iTempFile);
    iTempFile = 0;
#if TESTERCOMM_FILEPROFILING
    printf("--**-- [%12d] File closed. \n", ZTick());
#endif

    // move the temp file into the real file
    iResult = ZFileRename(pInterfaceData->strTempPathFile, pInterfaceData->strOutputPathFile);
    if (iResult != 0)
    {
        NetPrintf(("testercomm: error [%d=0x%X] renaming [%s] to [%s]\n",
            iResult, iResult, pInterfaceData->strTempPathFile, pInterfaceData->strOutputPathFile));
    }
#if TESTERCOMM_FILEPROFILING
    printf("--**-- [%12d] File renamed. \n", ZTick());
#endif

    // mark the last send time
    pState->uLastSendTime = ZTick();

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommConnect

    \Description
        Connect the host client communication module.

    \Input *pState              - pointer to host client comm module
    \Input *pParams             - startup parameters
    \Input bIsHost              - TRUE=host, FALSE=client
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 03/23/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommConnect(TesterCommT *pState, const char *pParams, uint32_t bIsHost)
{
    TesterCommFileT *pInterfaceData;
    uint32_t uControlDirSize;
    uint32_t uTempPathFileSize;
    uint32_t uInputPathFileSize;
    uint32_t uOutputPathFileSize;
    char strInputFile[256];
    char strOutputFile[256];
    char strControlDir[256];
    uint16_t aJson[512];

    // check for error conditions
    if ((pState == NULL) || (pParams == NULL) || (pState->pInterface->pData == NULL))
    {
        ZPrintf("testercomm: connect got invalid NULL pointer - pState [0x%X] pParams [0x%X]\n", pState, pParams);
        return(-1);
    }
    pInterfaceData = pState->pInterface->pData;

    // get the necessary startup parameters
    JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), pParams, -1);
    ds_memclr(strInputFile, sizeof(strInputFile));
    ds_memclr(strOutputFile, sizeof(strOutputFile));
    ds_memclr(strControlDir, sizeof(strControlDir));
    JsonGetString(JsonFind(aJson, "INPUTFILE"), strInputFile, sizeof(strInputFile), "");
    JsonGetString(JsonFind(aJson, "OUTPUTFILE"), strOutputFile, sizeof(strOutputFile), "");
    JsonGetString(JsonFind(aJson, "CONTROLDIR"), strControlDir, sizeof(strControlDir), "");

    // check for more error conditions
    if ((strlen(strInputFile) == 0) || (strlen(strOutputFile) == 0))
    {
        ZPrintf("testercomm: connect got invalid startup paths - INPUTFILE [%s] OUTPUTFILE [%s] CONTROLDIR [%s}\n",
            strInputFile, strOutputFile, strControlDir);
        return(-1);
    }

    // wipe out the current stored strings
    ds_memclr((void *)pInterfaceData->strControlDir, sizeof(pInterfaceData->strControlDir));
    ds_memclr((void *)pInterfaceData->strTempPathFile, sizeof(pInterfaceData->strTempPathFile));
    ds_memclr((void *)pInterfaceData->strInputPathFile, sizeof(pInterfaceData->strInputPathFile));
    ds_memclr((void *)pInterfaceData->strOutputPathFile, sizeof(pInterfaceData->strOutputPathFile));

    // get the available string size
    uControlDirSize     = sizeof(pInterfaceData->strControlDir)-1;
    uTempPathFileSize   = sizeof(pInterfaceData->strTempPathFile)-1;
    uInputPathFileSize  = sizeof(pInterfaceData->strInputPathFile)-1;
    uOutputPathFileSize = sizeof(pInterfaceData->strOutputPathFile)-1;

    // if we got nothing in for the control directory, use the default
    if (strlen(strControlDir) == 0)
    {
        strncat(pInterfaceData->strControlDir,     TESTERCOMM_PATH_DEFAULT, uControlDirSize);
        strncat(pInterfaceData->strTempPathFile,   TESTERCOMM_PATH_DEFAULT, uTempPathFileSize);
        strncat(pInterfaceData->strInputPathFile,  TESTERCOMM_PATH_DEFAULT, uInputPathFileSize);
        strncat(pInterfaceData->strOutputPathFile, TESTERCOMM_PATH_DEFAULT, uOutputPathFileSize);
    }
    else
    {
        strncat(pInterfaceData->strControlDir,     strControlDir, uControlDirSize);
        strncat(pInterfaceData->strTempPathFile,   strControlDir, uTempPathFileSize);
        strncat(pInterfaceData->strInputPathFile,  strControlDir, uInputPathFileSize);
        strncat(pInterfaceData->strOutputPathFile, strControlDir, uOutputPathFileSize);
    }

    // subtract off the amount of size we've used
    uTempPathFileSize   -= (uint32_t)strlen(pInterfaceData->strTempPathFile);
    uInputPathFileSize  -= (uint32_t)strlen(pInterfaceData->strInputPathFile);
    uOutputPathFileSize -= (uint32_t)strlen(pInterfaceData->strOutputPathFile);

    // attach the paths
    if (strlen(strControlDir) > 0)
    {
        strncat(pInterfaceData->strTempPathFile,   "/", uTempPathFileSize--);
        strncat(pInterfaceData->strInputPathFile,  "/",  uInputPathFileSize--);
        strncat(pInterfaceData->strOutputPathFile, "/",  uOutputPathFileSize--);
    }

    // attach an extra char for the tempfile.
    strncat(pInterfaceData->strTempPathFile,   "_", uTempPathFileSize--);

    // attach the filename
    strncat(pInterfaceData->strTempPathFile,   strOutputFile,       uTempPathFileSize);
    strncat(pInterfaceData->strInputPathFile,  strInputFile,        uInputPathFileSize);
    strncat(pInterfaceData->strOutputPathFile, strOutputFile,       uOutputPathFileSize);

    // some debugging
    ZPrintf("testercomm: connect ControlDir     [%s]\n", pInterfaceData->strControlDir);
    ZPrintf("testercomm: connect TempPathFile   [%s]\n", pInterfaceData->strTempPathFile);
    ZPrintf("testercomm: connect InputPathFile  [%s]\n", pInterfaceData->strInputPathFile);
    ZPrintf("testercomm: connect OutputPathFile [%s]\n", pInterfaceData->strOutputPathFile);

    // wipe out any existing output files at startup (remove any previous junk)
    ZFileDelete(pInterfaceData->strTempPathFile);
    ZFileDelete(pInterfaceData->strOutputPathFile);

    // dump anything from an incoming connection
    ZFileDelete(pInterfaceData->strInputPathFile);

    // set the tick time so we don't automatically get a timeout
    pState->uLastSendTime = ZTick();

    // done for now
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommUpdate

    \Description
        Give the host/client interface module some processor time.  Call this
        once in a while to pump the input and output pipes.

    \Input *pState - module state

    \Output    int32_t - 0 for success, error code otherwise

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommUpdate(TesterCommT *pState)
{
    int32_t iResult;

    if (pState == NULL)
        return(-1);

    // quit if we are suspended (don't do any more commands)
    if (pState->uSuspended)
        return(0);

    // check for outgoing and incoming data
    _TesterCommCheckOutput(pState);
    _TesterCommCheckInput(pState);

    // now call the callbacks for incoming messages
    iResult = ZListPopFront(pState->pInputData, &pState->LineData);
    while(iResult > 0)
    {
        // try to access the message map
        if (pState->MessageMap[pState->LineData.iType] != NULL)
        {
            // protect against recursion by suspending commands until this one completes
            TesterCommSuspend(pState);
            (pState->MessageMap[pState->LineData.iType])(pState, pState->LineData.strBuffer, pState->pMessageMapUserData[pState->LineData.iType]);
            TesterCommWake(pState);
        }

        // try to get the next chunk
        iResult = ZListPopFront(pState->pInputData, &pState->LineData);
    }
    
    // done
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _TesterCommDisconnect

    \Description
        Disconnect the host client communication module.

    \Input *pState - pointer to host client comm module
    
    \Output int32_t - 0=success, error code otherwise

    \Version 03/23/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterCommDisconnect(TesterCommT *pState)
{
    TesterCommFileT *pData;

    // check for error conditions
    if ((pState == NULL) || (pState->pInterface->pData == NULL))
    {
        return(-1);
    }
    pData = pState->pInterface->pData;

    // wipe out the current strings
    ds_memclr((void *)pData->strControlDir,     sizeof(pData->strControlDir));
    ds_memclr((void *)pData->strTempPathFile,   sizeof(pData->strTempPathFile));
    ds_memclr((void *)pData->strInputPathFile,  sizeof(pData->strInputPathFile));
    ds_memclr((void *)pData->strOutputPathFile, sizeof(pData->strOutputPathFile));

    // else return no error
    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterCommAttachFile

    \Description
        Attach file module function pointers to a tester comm module.

    \Input *pState - pointer to host client comm module
    
    \Output None

    \Version 05/02/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterCommAttachFile(TesterCommT *pState)
{
    if(pState == NULL)
        return;

    ZPrintf("testercomm: attaching FILE interface methods\n");
    
    pState->pInterface->CommConnectFunc    = &_TesterCommConnect;
    pState->pInterface->CommUpdateFunc     = &_TesterCommUpdate;
    pState->pInterface->CommDisconnectFunc = &_TesterCommDisconnect;
    pState->pInterface->pData = (TesterCommFileT *)ZMemAlloc(sizeof(TesterCommFileT));
    ds_memclr(pState->pInterface->pData, sizeof(TesterCommFileT));
}
