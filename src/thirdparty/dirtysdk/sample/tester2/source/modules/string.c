/*H********************************************************************************/
/*!
    \File string.c

    \Description
        Test the plat-str functionality.

    \Copyright
        Copyright (c) 2012 Electronic Arts

    \Version 10/01/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "libsample/zlib.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _StringCompareWild

    \Description
        Compare two strings with wildcard matching and print the result

    \Input bExpected    - expected result of comparison (true or false)
    \Input *pStrTest    - pointer to string to match against
    \Input *pStrWild    - pointer to wildcard string to match with
    \Input bNoCase      - TRUE for case insensitive, else FALSE

    \Output
        int32_t         - 0=expected result, 1=different result
    
    \Version 10/01/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StringCompareWild(uint32_t bExpected, const char *pStrTest, const char *pStrWild, uint8_t bNoCase)
{
    uint32_t bMatch;
    int32_t iResult;

    if (bNoCase)
    {
        bMatch = ds_stricmpwc(pStrTest, pStrWild) == 0 ? TRUE : FALSE;
    }
    else
    {
        bMatch = ds_strcmpwc(pStrTest, pStrWild) == 0 ? TRUE : FALSE;
    }

    iResult = (bMatch != bExpected) ? 1 : 0;
    ZPrintf("string: %s; strtest(%s) %s strwild(%s)\n", iResult ? "failure" : "success", pStrTest, bMatch ? "matched" : "did not match", pStrWild);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _StringTestCompareWild

    \Description
        Perform wild-card string comparison tests

    \Output
        int32_t         - 0=passed, else failed
    
    \Version 10/01/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _StringTestCompareWild(void)
{
    int32_t iResult = 0;

    ZPrintf("string: String wild-card comparison tests\n");

    iResult += _StringCompareWild(TRUE,  "gosca.ea.com", "*.ea.com", FALSE);
    iResult += _StringCompareWild(FALSE, "gosca.ea.com", "*.online.ea.com", FALSE);

    ZPrintf("string: String wild-card case-insensitive comparison tests\n");

    iResult += _StringCompareWild(TRUE,  "GOSCA.EA.COM", "*.ea.com", TRUE);
    iResult += _StringCompareWild(FALSE, "GOSCA.EA.COM", "*.online.ea.com", TRUE);

    ZPrintf("string: %d string test discrepencies\n", iResult);
    ZPrintf("string: ------------------------------------\n");
    return(iResult);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdString

    \Description
        Test platform string functions

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 10/01/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdString(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iResult = 0;

    ZPrintf("string: ------------------------------------------------\n");
    ZPrintf("string: Testing platform string (non-printing) functions\n");
    ZPrintf("string: ------------------------------------------------\n");
    
    iResult += _StringTestCompareWild();

    ZPrintf("string: Test results: %d total discrepencies\n", iResult);
    ZPrintf("string: ----------------------------------------------\n");    

    return(0);
}



