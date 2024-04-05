/*H********************************************************************************/
/*!
    \File base64.c

    \Description
        Test the Base64 encoder and decoder

    \Copyright
        Copyright (c) 2018 Electronic Arts Inc.

    \Version 12/16/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/base64.h"
#include "testermodules.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"
#include "libsample/zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdBase64

    \Description
        Test the Json formatter and parser

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 12/16/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdBase64(ZContext *argz, int32_t argc, char *argv[])
{
    // check for a file argument... if we have one, load and parse it
    if ((argc == 4) && !ds_stricmp(argv[1], "decode"))
    {
        char *pFileData, *pFileDecoded;
        ZFileT iOutFileId;
        int32_t iFileSize;

        if ((pFileData = ZFileLoad(argv[2], &iFileSize, FALSE)) != NULL)
        {
            int32_t iDecodedSize = Base64DecodedSize(iFileSize);
            if ((pFileDecoded = ZMemAlloc(iDecodedSize)) != NULL)
            {
                if (Base64Decode3(pFileData, iFileSize, pFileDecoded, iDecodedSize) > 0)
                {
                    if ((iOutFileId = ZFileOpen(argv[3], ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY|ZFILE_OPENFLAG_CREATE)) != ZFILE_INVALID)
                    {
                        ZFileWrite(iOutFileId, pFileDecoded, iDecodedSize);
                        ZFileClose(iOutFileId);
                        ZPrintf("%s: decoded %d bytes\n", argv[0], iDecodedSize);
                    }
                    else
                    {
                        ZPrintf("%s: could not open file '%s' for writing", argv[0], argv[3]);
                    }
                }
                else
                {
                    ZPrintf("%s: could not decode file '%s'\n", argv[0], argv[2]);
                }
                ZMemFree(pFileDecoded);
            }
            else
            {
                ZPrintf("%s: could not allocate %d bytes for base64 decode\n", argv[0], iDecodedSize);
            }
            ZMemFree(pFileData);
        }
        else
        {
            ZPrintf("%s: could not open file '%s' for reading\n", argv[0], argv[2]);
        }
    }

    return(0);
}


