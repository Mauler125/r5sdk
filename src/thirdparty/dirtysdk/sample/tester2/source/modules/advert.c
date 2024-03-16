/*H********************************************************************************/
/*!
    \File advert.c

    \Description
        Test the ProtoAdvt module.

    \Copyright
        Copyright (c) Electronic Arts 2003-2005.    ALL RIGHTS RESERVED.

    \Version 03/28/2003 (gschaefer) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protoadvt.h"

#include "libsample/zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

#define WATCH_BUFFER 4096

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

static ProtoAdvtRef *_Advert_pAdvert = NULL;

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdAdvert
    
    \Description
        Handle advertisement watcher
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output int32_t -
            
    \Version 1.0 02/17/2003 (jbrookes)
*/
/********************************************************************************F*/
static int32_t CmdAdvertWatch(ZContext *argz, int32_t argc, char **argv)
{
    char *pBuffer = (char *)argz;
    char strBuffer[WATCH_BUFFER];

    // get list of advertisements
    ProtoAdvtQuery(_Advert_pAdvert, "TESTER", "", strBuffer, sizeof(strBuffer), 0);
    if (strcmp(pBuffer, strBuffer) != 0)
    {
        ZPrintf("Advertising list:\n");
        ZPrintf("%s", strBuffer);
        ZPrintf("...end of list...\n");
        strcpy(pBuffer, strBuffer);
    }

    // keep running
    return(ZCallback(&CmdAdvertWatch, 500));
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdAdvert
    
    \Description
        Exercise the advertising module.
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output int32_t -
            
    \Version 1.0 02/17/2003 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdAdvert(ZContext *argz, int32_t argc, char **argv)
{
    int32_t iCount;
    char strBuffer[4096];

    // handle help
    if (argc == 0)
    {
        ZPrintf("%s list|add|del\n", argv[0]);
        return(0);
    }

    // startup advertising if needed
    if (_Advert_pAdvert == NULL)
    {
        _Advert_pAdvert = ProtoAdvtConstruct(16);
    }

    // handle listing advertisements
    if ((argc == 2) && (strcmp(argv[1], "list") == 0))
    {
        iCount = ProtoAdvtQuery(_Advert_pAdvert, "TESTER", "", strBuffer, sizeof(strBuffer), 0);
        if (iCount > 0)
        {
            ZPrintf("%s", strBuffer);
        }
    }

    // constant watch
    if ((argc == 2) && (strcmp(argv[1], "watch") == 0))
    {
        char *pBuffer = (char *)ZContextCreate(WATCH_BUFFER);
        pBuffer[0] = 0;
        return(ZCallback(&CmdAdvertWatch, 100));
    }

    // add an advertisement
    if ((argc > 2) && (strcmp(argv[1], "add") == 0))
    {
        ProtoAdvtAnnounce(_Advert_pAdvert, "TESTER", argv[2], "", "TCP:~1:1024\tUDP:~1:1024", 0);
    }

    // delete an advertisement
    if ((argc > 2) && (strcmp(argv[1], "del") == 0))
    {
        ProtoAdvtCancel(_Advert_pAdvert, "TESTER", argv[2]);
    }

    return(0);
}


