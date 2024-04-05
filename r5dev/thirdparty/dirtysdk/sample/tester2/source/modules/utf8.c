/*H********************************************************************************/
/*!
    \File utf8.c

    \Description
        Tester routine for UTF-8 encoding/decoding.

    \Copyright
        Copyright (c) 2003-2005 Electronic Arts Inc.

    \Version 03/25/2003 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/utf8.h"

#include "libsample/zfile.h"
#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

#define NUM_SUBTABLES (3)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/


// Private variables


/*
    UTF-8 to 7bit ASCII translation tables
*/

// Unicode: C0 Controls and Basic Latin
static unsigned char _TransBasicLatinTo7Bit[128] =
{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0xA, 255, 255, 0xD, 255, 255,     // 00-0F
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,     // 10-2F
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',    // 20-2F
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',     // 30-3F
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',     // 40-4F
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',    // 50-5F
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',     // 60-6F
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 255      // 70-7F
};

// Unicode: C1 Controls and Latin-1 Supplement
static unsigned char _TransLatin1To7Bit[] =
{
    // NOTE: 0x80-0x9F skipped
    255, '!', 'c', 'L', 'o', 'Y', '|', 255, 255, 255, 255, 255, 255, 255, 255, '-',     // A0-AF
    255, 255, 255, 255, '`', 'u', 'P', '.', ',', 255, 255, 255, 255, 255, 255, '?',     // B0-BF
    'A', 'A', 'A', 'A', 'A', 'A', 255, 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',     // C0-CF
    'D', 'N', 'O', 'O', 'O', 'O', 'O', 'x', 255, 'U', 'U', 'U', 'U', 'Y', 255, 'B',     // D0-DF
    'a', 'a', 'a', 'a', 'a', 'a', 255, 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',     // E0-EF
    255, 'n', 'o', 'o', 'o', 'o', 'o', '/', 255, 'u', 'u', 'u', 'u', 'y', 255, 'y'      // F0-FF
};

// Unicode: Latin Extended-A
static unsigned char _TransLatinExtATo7Bit[128] =
{
    'A', 'a', 'A', 'a', 'A', 'a', 'C', 'c', 'C', 'c', 'C', 'c', 'C', 'c', 'D', 'd',     // 100-10F
    'D', 'd', 'E', 'e', 'E', 'e', 'E', 'e', 'E', 'e', 'E', 'e', 'G', 'g', 'G', 'g',     // 110-11F
    'G', 'g', 'G', 'g', 'H', 'h', 'H', 'h', 'I', 'i', 'I', 'i', 'I', 'i', 'I', 'i',     // 120-12F
    'I', 'i', 255, 255, 'J', 'j', 'K', 'k', 'k', 'L', 'l', 'L', 'l', 'L', 'l', 'L',     // 130-13F
    'l', 'L', 'l', 'N', 'n', 'N', 'n', 'N', 'n', 'n', 'N', 'n', 'O', 'o', 'O', 'o',     // 140-14F
    'O', 'o', 255, 255, 'R', 'r', 'R', 'r', 'R', 'r', 'S', 's', 'S', 's', 'S', 's',     // 150-15F
    'S', 's', 'T', 't', 'T', 't', 'T', 't', 'U', 'u', 'U', 'u', 'U', 'u', 'U', 'u',     // 160-16F
    'U', 'u', 'U', 'u', 'W', 'w', 'Y', 'y', 'Y', 'Z', 'z', 'Z', 'z', 'Z', 'z', 'f',     // 170-17F
};

// master table
static Utf8TransTblT _TransTo7BitASCII[NUM_SUBTABLES+1] = 
{
    { 0x0000, 0x007F, _TransBasicLatinTo7Bit},  // handle Basic Latin set
    { 0x00A0, 0x00FF, _TransLatin1To7Bit    },  // handle Latin1
    { 0x0100, 0x017F, _TransLatinExtATo7Bit },  // handle LatinExtA

    { 0x0000, 0x0000, NULL,                 },  // NULL terminator
};

/*
    UTF-8 to 8bit ASCII translation tables
*/

static unsigned char _TransTo8BitASCIIa[256];

// master table
static Utf8TransTblT _TransTo8BitASCII[NUM_SUBTABLES+1] = 
{
    { 0x0000, 0x00FF, _TransTo8BitASCIIa    },  // handle Basic Latin set
    { 0x0000, 0x0000, NULL,                 },  // NULL terminator
};


// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdUtf8Init8BitTransTbl
    
    \Description
    
    \Input None.
    
    \Output
        None.
            
    \Version 03/25/2003 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdUtf8Init8BitTransTbl(void)
{
    int32_t iEntry;

    for (iEntry = 0; iEntry < 256; iEntry++)
    {
        _TransTo8BitASCIIa[iEntry] = (unsigned char)iEntry;
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    CmdUtf8
    
    \Description
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output
        int32_t         -
            
    \Version    1.0        03/25/03 (JLB) First Version
*/
/********************************************************************************F*/
int32_t CmdUtf8(ZContext *argz, int32_t argc, char **argv)
{
    int32_t iFileSize;
    char *pCharData;

    if (argc != 3)
    {
        ZPrintf("   test the utf8 module\n");
        ZPrintf("   usage: %s [l|r|s|7|8|c|u] [filename]\n", argv[0]);
        ZPrintf("      r = replace\n");
        ZPrintf("      s = strip\n");
        ZPrintf("      7 = translate UTF-8 to 7bit ASCII\n");
        ZPrintf("      8 = translate UTF-8 to 8bit ASCII\n");
        ZPrintf("      c = translate UTF-8 to UCS-2\n");
        ZPrintf("      u = translate UCS-2 to UTF-8\n");
        return(0);
    }

    _CmdUtf8Init8BitTransTbl();

    if ((pCharData = ZFileLoad(argv[2], &iFileSize, TRUE)) != NULL)
    {
        unsigned char *pUTF8Data = NULL;
        uint16_t *pUCS2Data = NULL;
        int32_t iUCS2Size = 0;
        int32_t iUTF8Size = 0;
        char cOption;

        cOption = argv[1][0];
        if (cOption != 'u')
        {
            ZPrintf("\n\n--Input--\n\n%s", pCharData);
        }
       
        if (cOption == 'l')
        {
            iUTF8Size = Utf8StrLen(pCharData);
            ZPrintf("  strlen('%s')=%d\n", pCharData, iUTF8Size);
        }
        else if (cOption == 'r')
        {
            Utf8Replace(pCharData, iFileSize, pCharData, '*');
            ZPrintf("\n\n--Output--\n\n%s", pCharData);
        }
        else if (cOption == 's')
        {
            Utf8Strip(pCharData, iFileSize, pCharData);
            ZPrintf("\n\n--Output--\n\n%s", pCharData);
        }
        else if (cOption == '7')
        {
            Utf8TranslateTo8Bit(pCharData, iFileSize, pCharData, '*', _TransTo7BitASCII);
            ZPrintf("\n\n--7 Bit ASCII--\n\n%s", pCharData);
        }
        else if (cOption == '8')
        {
            Utf8TranslateTo8Bit(pCharData, iFileSize, pCharData, '*', _TransTo8BitASCII);
            ZPrintf("\n\n--8 Bit ASCII--\n\n%s", pCharData);
        }
        else if (cOption == 'c')
        {
            pUCS2Data = (uint16_t *)ZMemAlloc(iFileSize*sizeof(int16_t));
            iUCS2Size = Utf8DecodeToUCS2(pUCS2Data, iFileSize, pCharData) * sizeof(int16_t);
        }
        else if (cOption == 'u')
        {
            pUTF8Data = (unsigned char *)ZMemAlloc(iFileSize);
            iUTF8Size = Utf8EncodeFromUCS2((char *)pUTF8Data, iFileSize, (const uint16_t *)pCharData);
        }
        else
        {
            ZPrintf("Unsupported option '%c'\n",cOption);
        }

        // free file buffer
        ZMemFree(pCharData);

        // write out UCS-2 data, if any
        if ((pUCS2Data != NULL) && (iUCS2Size > 0))
        {
            ZFileSave("out_ucs2.txt", (const char *)pUCS2Data, iUCS2Size, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY);
            ZMemFree(pUCS2Data);
        }

        // write out UTF-8 data, if any
        if ((pUTF8Data != NULL) && (iUTF8Size > 0))
        {
            ZFileSave("out_utf8.txt", (char *)pUTF8Data, iUTF8Size, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY);
            ZMemFree(pUTF8Data);
        }
    }
    else
    {
        ZPrintf("Unable to open input file.\n");
    }

    return(0);
}
