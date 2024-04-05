/*H********************************************************************************/
/*!
    \File testerprofile.h

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

#ifndef _testerprofile_h
#define _testerprofile_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define TESTERPROFILE_ERROR_NONE                     (0)    //!< no error
#define TESTERPROFILE_ERROR_NULLPOINTER             (-1)    //!< invalid pointer used
#define TESTERPROFILE_ERROR_INVALIDFILENAME         (-2)    //!< bad filename
#define TESTERPROFILE_ERROR_FILEALREADYOPEN         (-3)    //!< file already opened
#define TESTERPROFILE_ERROR_FILEOPEN                (-4)    //!< could not open requested file
#define TESTERPROFILE_ERROR_NOSUCHENTRY             (-5)    //!< entry not found

#define TESTERPROFILE_NUMPROFILES_DEFAULT           (16)                //!< max number of profiles to keep around

#define TESTERPROFILE_PROFILEFILENAME_SIZEDEFAULT   (256)               //!< size of the profile filename string
#define TESTERPROFILE_PROFILEFILENAME_VALUEDEFAULT  (".\\profile.txt")  //!< profile filename

#define TESTERPROFILE_PROFILENAME_SIZEDEFAULT       (32)                //!< size of each profile name
#define TESTERPROFILE_PROFILENAME_VALUEDEFAULT      ("<empty>")         //!< default name for the profile if none specified
#define TESTERPROFILE_PROFILENAME_TAGVALUE          ("NAME")            //!< the tag the data will be stored under

#define TESTERPROFILE_CONTROLDIR_SIZEDEFAULT        (256)               //!< size of the directory entry
#define TESTERPROFILE_CONTROLDIR_VALUEDEFAULT       (".\\")             //!< default file sharing directory
#define TESTERPROFILE_CONTROLDIR_TAGVALUE           ("CONTROLDIR")      //!< the tag the data will be stored under

#define TESTERPROFILE_COMMANDLINE_SIZEDEFAULT       (256)               //!< size of the optional command line params
#define TESTERPROFILE_COMMANDLINE_VALUEDEFAULT      ("")                //!< default command line params
#define TESTERPROFILE_COMMANDLINE_TAGVALUE          ("COMMANDLINE")     //!< the tag the data will be stored under

#define TESTERPROFILE_LOGDIRECTORY_SIZEDEFAULT      (256)               //!< size of the file log location
#define TESTERPROFILE_LOGDIRECTORY_VALUEDEFAULT     (".\\")             //!< location to create a logfile at
#define TESTERPROFILE_LOGDIRECTORY_TAGVALUE         ("LOGDIR")          //!< the tag the data will be stored under

#define TESTERPROFILE_HOSTNAME_SIZEDEFAULT          (32)                //!< size of the hostname
#define TESTERPROFILE_HOSTNAME_VALUEDEFAULT         ("")                //!< default hostname
#define TESTERPROFILE_HOSTNAME_TAGVALUE             ("HOSTNAME")        //!< the tag the data will be stored under

#define TESTERPROFILE_LOGENABLE_VALUEDEFAULT        (1)                 //!< log to a file by default
#define TESTERPROFILE_LOGENABLE_TAGVALUE            ("LOGENABLE")       //!< the tag the data will be stored under

#define TESTERPROFILE_NETWORKSTART_VALUEDEFAULT     (1)                 //!< bring up the network by default
#define TESTERPROFILE_NETWORKSTART_TAGVALUE         ("NETSTART")        //!< the tag the data will be stored under

#define TESTERPROFILE_DEFAULTPROFILE_TAGVALUE       ("DEFAULT")         //!< the tag the data will be stored under

#define TESTERPROFILE_HISTORYFILE_SIZEDEFAULT       (288)               //!< size of the history filename
#define TESTERPROFILE_HISTORYFILE_VALUEDEFAULT      ("")                //!< default value
#define TESTERPROFILE_HISTORYFILE_TAGVALUE          ("HISTORYFILE")     //!< tag the data will be stored under

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// module state container
typedef struct TesterProfileT TesterProfileT;

// here's the actual profile data structure
typedef struct TesterProfileEntryT
{
    char strProfileName[TESTERPROFILE_PROFILENAME_SIZEDEFAULT];      //!< profile name
    char strControlDirectory[TESTERPROFILE_CONTROLDIR_SIZEDEFAULT];  //!< control directory name
    char strCommandLine[TESTERPROFILE_COMMANDLINE_SIZEDEFAULT];      //!< command line options
    char strLogDirectory[TESTERPROFILE_LOGDIRECTORY_SIZEDEFAULT];    //!< logfile directory
    char strHostname[TESTERPROFILE_HOSTNAME_SIZEDEFAULT];            //!< hostname
    char strHistoryFile[TESTERPROFILE_HISTORYFILE_SIZEDEFAULT];      //!< history filename
    char strLogFileName[TESTERPROFILE_LOGDIRECTORY_SIZEDEFAULT];     //!< Log file name 
    unsigned char uLogEnable;                           //!< 1 to enable file logging
    unsigned char uNetworkStartup;                      //!< 1 for network startup on connect
    unsigned char uDefault;                             //!< 1 for the default profile, 0 otherwise
    unsigned char uPad1;                                //!< pad variable
} TesterProfileEntryT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a tester profile manager
TesterProfileT *TesterProfileCreate(void);

// connect the profile manager to a set of profiles in the given profile file
int32_t TesterProfileConnect(TesterProfileT *pState, const char *pFilename);

// return a specifically indexed profile into a json buffer
int32_t TesterProfileGet(TesterProfileT *pState, int32_t iIndex, TesterProfileEntryT *pDest);

// return a specifically indexed profile into a json buffer
int32_t TesterProfileGetJson(TesterProfileT *pState, int32_t iIndex, char *pDest, int32_t iSize);

// add a profile to the list of profiles
int32_t TesterProfileAdd(TesterProfileT *pState, TesterProfileEntryT *pEntry);

// delete a profile from the list of profiles
int32_t TesterProfileDelete(TesterProfileT *pState, const char *pName);

// set a particular profile as a default profile (loads first next time)
int32_t TesterProfileSetDefaultName(TesterProfileT *pState, const char *pProfileName);

// get the index of the default profile
int32_t TesterProfileGetDefaultIndex(TesterProfileT *pState);

// save the profile file (manual save - will happen automatically on disconnect)
int32_t TesterProfileSave(TesterProfileT *pState);

// save the current profiles and disconnect the profile manager from the profile set
int32_t TesterProfileDisconnect(TesterProfileT *pState);

// destroy a tester profile object
void TesterProfileDestroy(TesterProfileT *pState);

#ifdef __cplusplus
};
#endif

#endif // _testerprofile_h

