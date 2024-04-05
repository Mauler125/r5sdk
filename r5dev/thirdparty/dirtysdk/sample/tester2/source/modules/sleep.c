/*H********************************************************************************/
/*!
    \File net.c

    \Description
        Handles SLEEP for tester2

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/11/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"

#include "libsample/zlib.h"

#include "testerregistry.h"
#include "testercomm.h"
#include "testerclientcore.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdSleep

    \Description
        Sleep for a while

    \Input *argz    - environment
    \Input argc     - number of args
    \Input *argv[]  - argument list
    
    \Output int32_t - standard return code

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t CmdSleep(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iTime, iTick;
    TesterClientCoreT *pCore;
    TesterCommT *pComm;

    //milliseconds to sleep before pumping I/O..
    #define TICK_STEP 100

    pCore = (TesterClientCoreT *)TesterRegistryGetPointer("CORE");
    
    // don't recurse
    if (argc < 2)
    {
        ZPrintf("   sleep for the given amount of time\n");
        ZPrintf("   usage: %s <seconds>\n", argv[0]);
    }
    else
    {
        iTime = atoi(argv[1]) *1000;
        ZPrintf("%s: sleep for [%d] seconds.\n", argv[0], iTime/1000);
        for (iTick = 0; iTick < iTime; iTick += TICK_STEP)
        {
            // pump output when sleeping; This makes "sleep" client only.
            TesterClientCoreIdle(pCore);
            ZSleep(TICK_STEP); //NOTE: ZSleep calls NetConnIdle..
        }
    }

    // now, reset comm timer since we were sleeping..
    if ((pComm = (TesterCommT *)TesterRegistryGetPointer("COMM")) != NULL)
    {
        // slept - so bump timeout..
        pComm->uLastSendTime = ZTick();
    }
    return(0);
}
