/*H********************************************************************************/
/*!
    \File testerprofile.c

    \Description
        Maintain user profiles for the tester application.

    \Notes
        A profile is selected, created, or deleted after client GUI start 
        and before anything else happens.  Currently, the following items 
        will be stored in a tester2 profile: command history, network startup, 
        and lobby parameters.  Profiles will be stored in a single file on the 
        client’s disk, the elements of the profile encoded into tagfields. 

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/18/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zmem.h"
#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "testerprofile.h"
#include "testerregistry.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

// module state
struct TesterProfileT
{
    TesterProfileEntryT Profiles[TESTERPROFILE_NUMPROFILES_DEFAULT];    //!< profile entry list
    uint32_t        uProfileTail;                                   //!< tail index for the profile list
    char    strFilename[TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT];     //!< filename for the profile file
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _TesterProfileGenerateHistoryFilename

    \Description
        Use an entry's name to generate a history file name

    \Input *pState    - module state
    
    \Output int32_t - error codes, 0 if success

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterProfileGenerateHistoryFilename(TesterProfileEntryT *pEntry)
{
    int32_t iNumChars, iChecksum;
    const char strFileSuffix[] = ".history.txt";
    char strChecksum[16];
    char cReplace;
    

    // check for errors
    if(pEntry == NULL)
        return;

    // create the history filename
    iChecksum=0;
    ds_memclr(strChecksum, sizeof(strChecksum));
    ds_memclr(pEntry->strHistoryFile, sizeof(pEntry->strHistoryFile));
    for(iNumChars = 0; 
        (iNumChars < (signed)sizeof(pEntry->strProfileName)) && (pEntry->strProfileName[iNumChars] != 0);
        iNumChars++)
    {
        cReplace = pEntry->strProfileName[iNumChars];
        iChecksum += (int32_t)cReplace;
        if( ((cReplace >= '0') && (cReplace <= '9')) ||
            ((cReplace >= 'a') && (cReplace <= 'z')) ||
            ((cReplace >= 'A') && (cReplace <= 'Z')) ||
            ((cReplace >= '0') && (cReplace <= '9')) )
        {
            // character OK
        }
        else
        {
            cReplace = '_';
        }
        pEntry->strHistoryFile[iNumChars] = cReplace;
    }
    sprintf(strChecksum, "%d", iChecksum);
    strncat(pEntry->strHistoryFile, strChecksum,   sizeof(pEntry->strHistoryFile) - strlen(pEntry->strHistoryFile) - 1);
    strncat(pEntry->strHistoryFile, strFileSuffix, sizeof(pEntry->strHistoryFile) - strlen(pEntry->strHistoryFile) - 1);
}


/*F********************************************************************************/
/*!
    \Function _TesterProfileKillAllEntries

    \Description
        Delete all the profile entries in the profile list

    \Input *pState - module state to delete all the entries from
    
    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterProfileKillAllEntries(TesterProfileT *pState)
{
    pState->uProfileTail = 0;
    ds_memclr(pState->Profiles, sizeof(pState->Profiles));
}

/*F********************************************************************************/
/*!
    \Function _TesterProfileParseLine

    \Description
        Take an incoming tagfield and create a profile structure from it

    \Input  *pData - incoming data buffer
    \Input *pEntry - profile entry structure to put the data into
    
    \Output None

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterProfileParseLine(char *pData, TesterProfileEntryT *pEntry)
{
    uint16_t aJson[512];

    // check error conditions
    if((pData == NULL) || (pEntry == NULL))
        return;

    // wipe the supplied structure
    ds_memclr(pEntry, sizeof(*pEntry));

    JsonParse(aJson, sizeof(aJson)/sizeof(*aJson), pData, -1);

    // populate with data
    JsonGetString(JsonFind(aJson, TESTERPROFILE_PROFILENAME_TAGVALUE), pEntry->strProfileName, sizeof(pEntry->strProfileName), TESTERPROFILE_PROFILENAME_VALUEDEFAULT);
    JsonGetString(JsonFind(aJson, TESTERPROFILE_CONTROLDIR_TAGVALUE), pEntry->strControlDirectory, sizeof(pEntry->strControlDirectory), TESTERPROFILE_CONTROLDIR_VALUEDEFAULT);
    JsonGetString(JsonFind(aJson, TESTERPROFILE_COMMANDLINE_TAGVALUE), pEntry->strCommandLine, sizeof(pEntry->strCommandLine), TESTERPROFILE_COMMANDLINE_VALUEDEFAULT);
    JsonGetString(JsonFind(aJson, TESTERPROFILE_LOGDIRECTORY_TAGVALUE), pEntry->strLogDirectory, sizeof(pEntry->strLogDirectory), TESTERPROFILE_LOGDIRECTORY_VALUEDEFAULT);
    JsonGetString(JsonFind(aJson, TESTERPROFILE_HOSTNAME_TAGVALUE), pEntry->strHostname, sizeof(pEntry->strHostname), TESTERPROFILE_HOSTNAME_VALUEDEFAULT);
    JsonGetString(JsonFind(aJson, TESTERPROFILE_HISTORYFILE_TAGVALUE), pEntry->strHistoryFile, sizeof(pEntry->strHistoryFile), TESTERPROFILE_HISTORYFILE_VALUEDEFAULT);
    pEntry->uLogEnable = (uint8_t)JsonGetInteger(JsonFind(aJson, TESTERPROFILE_LOGENABLE_TAGVALUE), TESTERPROFILE_LOGENABLE_VALUEDEFAULT);
    pEntry->uNetworkStartup = (uint8_t)JsonGetInteger(JsonFind(aJson, TESTERPROFILE_NETWORKSTART_TAGVALUE), TESTERPROFILE_NETWORKSTART_VALUEDEFAULT);
    pEntry->uDefault = (uint8_t)JsonGetInteger(JsonFind(aJson, TESTERPROFILE_DEFAULTPROFILE_TAGVALUE), 0);

    // create a history file name
    _TesterProfileGenerateHistoryFilename(pEntry);
}

/*F********************************************************************************/
/*!
    \Function _TesterProfileParseFile

    \Description
        Take incoming data (from a file) and parse into profile structures

    \Notes
        This function is destructive and will modify the contents at pData.

    \Input   *pState - module state
    \Input    *pData - incoming data buffer
    \Input     iSize - amount of data in the buffer
    
    \Output None

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
static void _TesterProfileParseFile(TesterProfileT *pState, char *pData)
{
    const char strSep[] = {"\r\n"};
    char *pDataPtr;

    // wipe out the previous list in case it exists
    _TesterProfileKillAllEntries(pState);

    // walk the pData buffer and parse each incoming line
    pDataPtr = (char *)strtok(pData, strSep);
    while(pDataPtr)
    {
        // get an entry based on the line
        _TesterProfileParseLine(pDataPtr, &pState->Profiles[pState->uProfileTail]);
        pDataPtr = (char *)strtok(NULL, strSep);
        pState->uProfileTail++;
    }
}


/*F********************************************************************************/
/*!
    \Function _TesterProfileReadFile

    \Description
        Read a file containing profiles and store it in the profile list.

    \Input *pState    - module state
    
    \Output int32_t - error codes, 0 if success

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _TesterProfileReadFile(TesterProfileT *pState)
{
    char *pProfileString;
    int32_t iFileSize;

    // load the file in
    pProfileString = ZFileLoad(pState->strFilename, &iFileSize, 0);
    if(pProfileString == NULL)
    {
        ZPrintf("testerprofile: TesterProfileConnect error opening file [%s]\n", pState->strFilename);
        return(TESTERPROFILE_ERROR_FILEOPEN);
    }

    // parse the file out
    _TesterProfileParseFile(pState, pProfileString);

    // don't forget to dump the memory when done
    ZMemFree(pProfileString);

    // quit
    return(TESTERPROFILE_ERROR_NONE);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TesterProfileCreate

    \Description
        Create a tester profile manager.

    \Input None
    
    \Output TesterProfileT * - allocated module state pointer

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
TesterProfileT *TesterProfileCreate(void)
{
    TesterProfileT *pState = (TesterProfileT *)ZMemAlloc(sizeof(TesterProfileT));
    ds_memclr(pState, sizeof(TesterProfileT));
    TesterRegistrySetPointer("PROFILE", pState);
    return(pState);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileConnect

    \Description
        Connect the profile manager to a set of profiles in a file.

    \Input *pState    - module state
    \Input *pFilename - filename to try to read
    
    \Output int32_t - error codes, 0 if success

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileConnect(TesterProfileT *pState, const char *pFilename)
{
    
    // check for error conditions
    if (pState == NULL)
        return(TESTERPROFILE_ERROR_NULLPOINTER);
    if (pFilename == NULL)
        return(TESTERPROFILE_ERROR_INVALIDFILENAME);
    if (pState->strFilename[0] != 0)
        return(TESTERPROFILE_ERROR_FILEALREADYOPEN);

    // copy the filename in
    ds_strnzcpy(pState->strFilename, pFilename, sizeof(pState->strFilename));

    // read the contents of the requested file
    _TesterProfileReadFile(pState);

    // done
    return(TESTERPROFILE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileAdd

    \Description
        Add a profile to the list

    \Input *pState          - module state
    \Input *pEntry          - profile entry to add
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileAdd(TesterProfileT *pState, TesterProfileEntryT *pEntry) 
{
    TesterProfileEntryT *pTarget = NULL;
    uint32_t uLoop, uNameLen, uStoredNameLen;
    char *pStoredProfileName;

    // check for errors
    if((pState == NULL) || (pEntry == NULL))
    {
        return(TESTERPROFILE_ERROR_NULLPOINTER);
    }

    // make sure we have a valid history filename
    _TesterProfileGenerateHistoryFilename(pEntry);

    // walk the list and look for a place to put it
    // watch for a matching profile name as well
    uNameLen = (uint32_t)strlen(pEntry->strProfileName);
    for(uLoop = 0; (uLoop < pState->uProfileTail) && (pTarget == NULL); uLoop++)
    {
        pStoredProfileName = pState->Profiles[uLoop].strProfileName;
        uStoredNameLen = (uint32_t)strlen(pStoredProfileName);
        // if it matches, we have a target
        if((uStoredNameLen == uNameLen) &&
            (strcmp(pStoredProfileName, pEntry->strProfileName) == 0))
        {
            pTarget = &pState->Profiles[uLoop];
        }
    }
    
    // if we don't have a match, see if we can make some room at the end
    if(pTarget == NULL)
    {
        // no match - see if we have room
        if(pState->uProfileTail < TESTERPROFILE_NUMPROFILES_DEFAULT)
        {
            pTarget = &pState->Profiles[pState->uProfileTail];
            pState->uProfileTail++;
        }
    }

    // copy all the parameters in if we found a spot
    if(pTarget != NULL)
    {
        ds_memcpy(pTarget, pEntry, sizeof(TesterProfileEntryT));
    }

    return(TESTERPROFILE_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterProfileDelete

    \Description
        Remove a profile from the list

    \Input *pState       - module state
    \Input *pName        - profile to nuke
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileDelete(TesterProfileT *pState, const char *pName)
{
    uint32_t uLoop, uNameLen, uStoredNameLen;
    char *pStoredProfileName;

    // check for errors
    if((pState == NULL) || (pName == NULL))
        return(TESTERPROFILE_ERROR_NULLPOINTER);

    // nuke the entry in question
    uNameLen = (uint32_t)strlen(pName);
    for(uLoop = 0; uLoop < pState->uProfileTail; uLoop++)
    {
        // if it matches, we have a target
        pStoredProfileName = (pState->Profiles[uLoop]).strProfileName;
        uStoredNameLen = (uint32_t)strlen(pStoredProfileName);
        if((uStoredNameLen == uNameLen) &&
          (strcmp(pStoredProfileName, pName) == 0))
        {
            // nuke the entry so we won't save it
            ds_memclr(&pState->Profiles[uLoop],sizeof(TesterProfileEntryT));
        }
    }

    // now force a save (save won't dump a NULL entry)
    TesterProfileSave(pState);
    // and re-read the file
    _TesterProfileReadFile(pState);

    return(TESTERPROFILE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileSetDefaultName

    \Description
        Set a particular profile as the default profile

    \Input *pState       - module state
    \Input *pProfileName - profile to set as default
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileSetDefaultName(TesterProfileT *pState, const char *pProfileName)
{
    uint32_t uStoredNameLen, uNameLen;
    char *pStoredProfileName;
    uint32_t uLoop;
    
    // check for errors first
    if((pState == NULL) || (pProfileName == NULL))
        return(TESTERPROFILE_ERROR_NULLPOINTER);

    uNameLen = (uint32_t)strlen(pProfileName);
    for(uLoop = 0; uLoop < TESTERPROFILE_NUMPROFILES_DEFAULT; uLoop++)
    {
        pState->Profiles[uLoop].uDefault = 0;
        pStoredProfileName = pState->Profiles[uLoop].strProfileName;
        uStoredNameLen = (uint32_t)strlen(pStoredProfileName);
        if((uNameLen == uStoredNameLen) && 
           (strcmp(pProfileName, pStoredProfileName) == 0))
        {
            pState->Profiles[uLoop].uDefault = 1;
        }
    }

    // no error
    return(TESTERPROFILE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileGetDefaultIndex

    \Description
        Return the index of the default profile

    \Input *pState       - module state
    
    \Output int32_t - index of default profile, 0 if no default

    \Version 03/28/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileGetDefaultIndex(TesterProfileT *pState)
{
    uint32_t uLoop;
    int32_t iIndex = TESTERPROFILE_ERROR_NOSUCHENTRY;

    if(pState == NULL)
        return(TESTERPROFILE_ERROR_NULLPOINTER);

    // loop while less than total number and not a default entry
    for(uLoop = 0; uLoop < TESTERPROFILE_NUMPROFILES_DEFAULT; uLoop++)
    {
        if(pState->Profiles[uLoop].uDefault != 0)
            iIndex = uLoop;
    }
    return(iIndex);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileSave

    \Description
        Save all profiles to disk.

    \Notes
        Called automatically on disconnect - no need to call this function
        unless special functionality is desired.  Will not save NULL entries.

    \Input *pState       - module state
    
    \Output int32_t - 0 for success, error code otherwise

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileSave(TesterProfileT *pState)
{
    char strProfile[1024];
    ZFileT iFilePointer;
    int32_t iEntryNum;

    // open the file for create (no append)
    iFilePointer = ZFileOpen(pState->strFilename, ZFILE_OPENFLAG_CREATE | ZFILE_OPENFLAG_WRONLY);
    if (iFilePointer < 0)
    {
        return(TESTERPROFILE_ERROR_FILEOPEN);
    }

    // loop through all entries and write the valid ones out
    for (iEntryNum = 0; iEntryNum < TESTERPROFILE_NUMPROFILES_DEFAULT; iEntryNum++)
    {
        // will return an error if the entry is NULL or nuked
        if (TesterProfileGetJson(pState, iEntryNum, strProfile, sizeof(strProfile)) >= 0)
        {
            // write it to the file
            ZFileWrite(iFilePointer, (void *)strProfile, (int32_t)strlen(strProfile)); 
        }
    }

    // close the file
    ZFileClose(iFilePointer);

    // done
    return(TESTERPROFILE_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileGet

    \Description
        Return a specific profile index into a tagfield string buffer.

    \Input *pState - module state
    \Input  iIndex - 0-based index of the profile to get
    \Input *pDest  - destination buffer
    
    \Output int32_t - index number of retreived entry (>=0) , error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileGet(TesterProfileT *pState, int32_t iIndex, TesterProfileEntryT *pDest)
{
    TesterProfileEntryT *pEntry;

    // see if we want the default entry
    if (iIndex == -1)
        iIndex = TesterProfileGetDefaultIndex(pState);

    // check for error conditions
    if ((pState == NULL) || (pDest == NULL))
        return(TESTERPROFILE_ERROR_NULLPOINTER);
    else if (iIndex < -1)
        return(TESTERPROFILE_ERROR_NOSUCHENTRY);

    // set the entry pointer
    pEntry = &pState->Profiles[iIndex];

    // if the entry has been nuked, don't do anything
    if(pEntry->strProfileName[0] == 0)
        return(TESTERPROFILE_ERROR_NOSUCHENTRY);

    // copy the data in
    ds_memcpy(pDest, pEntry, sizeof(TesterProfileEntryT));

    // done
    return(iIndex);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileGetJson

    \Description
        Return a specific profile index into a json string buffer.

    \Input *pState - module state
    \Input  iIndex - 0-based index of the profile to get
    \Input *pDest  - destination buffer
    \Input  iSize  - size of the destination buffer

    \Output int32_t - index number of retreived entry (>=0) , error code otherwise

    \Version 03/29/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileGetJson(TesterProfileT *pState, int32_t iIndex, char *pDest, int32_t iSize)
{
    TesterProfileEntryT Entry;
    int32_t iResult;

    if (pState == NULL)
        return(TESTERPROFILE_ERROR_NULLPOINTER);

    iResult = TesterProfileGet(pState, iIndex, &Entry);
    if (iResult < 0)
        return(iResult);

    // otherwise dump the json into the buffer
    JsonInit(pDest, iSize, 0);
    JsonAddStr(pDest, TESTERPROFILE_PROFILENAME_TAGVALUE, Entry.strProfileName);
    JsonAddStr(pDest, TESTERPROFILE_HOSTNAME_TAGVALUE, Entry.strHostname);
    JsonAddStr(pDest, TESTERPROFILE_CONTROLDIR_TAGVALUE, Entry.strControlDirectory);
    JsonAddStr(pDest, TESTERPROFILE_COMMANDLINE_TAGVALUE, Entry.strCommandLine);
    JsonAddStr(pDest, TESTERPROFILE_LOGDIRECTORY_TAGVALUE, Entry.strLogDirectory);
    JsonAddStr(pDest, TESTERPROFILE_HISTORYFILE_TAGVALUE, Entry.strHistoryFile);
    JsonAddInt(pDest, TESTERPROFILE_LOGENABLE_TAGVALUE, Entry.uLogEnable);
    JsonAddInt(pDest, TESTERPROFILE_NETWORKSTART_TAGVALUE, Entry.uNetworkStartup);
    JsonAddInt(pDest, TESTERPROFILE_DEFAULTPROFILE_TAGVALUE, Entry.uDefault);
    pDest = JsonFinish(pDest);

    // done
    return(iIndex);
}


/*F********************************************************************************/
/*!
    \Function TesterProfileDisconnect

    \Description
        Disconnect from a set of profiles and save the current profiles.

    \Input *pState - module state

    \Output int32_t - result of trying to save the profiles (TesterProfileSave())

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterProfileDisconnect(TesterProfileT *pState)
{
    int32_t iResult;

    // check for valid state - not really an error
    if (pState == NULL)
        return(TESTERPROFILE_ERROR_NONE);
    // check to see if we're already disconnected
    if (pState->strFilename[0] == 0)
        return(TESTERPROFILE_ERROR_NONE);

    // make a best effort at saving the file
    iResult = TesterProfileSave(pState);
    if (iResult)
    {
        ZPrintf("testerprofile: TesterProfileDisconnect: error saving file [%s]\n",
            pState->strFilename);
    }

    // kill all the entries in the profile list
    _TesterProfileKillAllEntries(pState);

    // wipe out the filename to signify we've disconnected
    ds_memclr(pState->strFilename, sizeof(pState->strFilename));

    // return the fclose error code
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function TesterProfileDestroy

    \Description
        Destroy and allocated tester profile object.

    \Input *pState - module state
    
    \Output None

    \Version 03/18/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterProfileDestroy(TesterProfileT *pState)
{
    if(pState)
    {
        TesterProfileDisconnect(pState);
        ZMemFree(pState);
    }
    TesterRegistrySetPointer("PROFILE", NULL);
}

