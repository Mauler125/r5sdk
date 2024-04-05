/*H********************************************************************************/
/*!
    \File json.c

    \Description
        Test the JSON formatter and parser.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 12/11/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "testermodules.h"

#include "libsample/zlib.h"
#include "libsample/zfile.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

static const char *_pSimpleArrayStringTest = \
"{" \
    "[" \
    "\"abcd\"," \
    "\"efgh\"," \
    "\"ijkl\"," \
    "\"mnop\""  \
    "]" \
"};";

static const char *_pSimpleArrayNumberTest = \
"{" \
    "[" \
    "101," \
    "5005," \
    "7," \
    "32768"  \
    "]" \
"};";


/*** Private Functions ************************************************************/

static void _SimpleDecode(void)
{
    char strBuffer[1024], *pBuffer;
    uint16_t buff[4096];
    const char *pData = "{\"isAllowed\":false,\"reasons\":[{\"reason\":\"CheckFailed\"}]}";
    const char *pResult;
    uint8_t bAllowed;

    JsonParse(buff, sizeof(buff) / sizeof(buff[0]), pData, (int32_t)strlen(pData));
    pResult = JsonFind(buff, "isAllowed");
    bAllowed = (JsonGetBoolean(pResult, 0) == 0) ? FALSE : TRUE;

    // encode some Json
    // "id": 2533274790412952,
    // "name": "TestPlayer0",
    // "seatIndex": -1
    JsonInit(strBuffer, sizeof(strBuffer), JSON_FL_WHITESPACE);
    JsonObjectStart(strBuffer, NULL);
    JsonAddInt(strBuffer, "id", 2533274790412952LL);
    JsonAddStr(strBuffer, "name", "TestPlayer0");
    JsonAddInt(strBuffer, "seatIndex", -1);
    JsonObjectEnd(strBuffer);
    pBuffer = JsonFinish(strBuffer);
    ZPrintf("encoded Json:\n------------\n%s\n-----------\n", pBuffer);
}

static void _SimpleEncode(void)
{
    char strBuffer[1024], *pBuffer;

    /* now something a little more challenging:

    {
    "body":
    {
    "attachments":null,
    "partnerData":
    {
    "MultiplayerMessageType":"YourTurn",
    "SessionId": "C8921C4E-4FC2-439E-9368-5D26889BD1BB"
    }
    },
    "header":
    {
    "attributes":null,
    "expiration":"2011-10-11T23:59:59.9999999",
    "id":null,
    "messageType":"Multiplayer",
    "recipients":[{"userId":"GoTeamEmily","userType":"Gamertag"},{"userId":"Longstreet360","userType":"Gamertag"}]],
    "sender":"Striker",
    "senderPlatform":null,
    "sent":"2011-10-13T16:40:58.1890842-07:00",
    "targetPlatforms":["MP3"],
    "title":1297287259
    }
    }

    */
    JsonInit(strBuffer, sizeof(strBuffer), JSON_FL_WHITESPACE);
    JsonObjectStart(strBuffer, "body");
    JsonAddStr(strBuffer, "attachments", NULL);
    JsonObjectStart(strBuffer, "partnerData");
    JsonAddStr(strBuffer, "MultiplayerMessageType", "YourTurn");
    JsonAddStr(strBuffer, "SessionId", "C8921C4E-4FC2-439E-9368-5D26889BD1BB"); // AddGuid()?
    JsonObjectEnd(strBuffer);
    JsonObjectEnd(strBuffer);
    JsonObjectStart(strBuffer, "header");
    JsonAddStr(strBuffer, "attributes", NULL);
    JsonAddStr(strBuffer, "expiration", "2011-10-11T23:59:59.9999999"); // AddDate()?
    JsonAddStr(strBuffer, "id", NULL);
    JsonAddStr(strBuffer, "messageType", "Multiplayer");
    JsonArrayStart(strBuffer, "recipients");
    JsonObjectStart(strBuffer, NULL);
    JsonAddStr(strBuffer, "userId", "GoTeamEmily");
    JsonAddStr(strBuffer, "userType", "Gamertag");
    JsonObjectEnd(strBuffer);
    JsonObjectStart(strBuffer, NULL);
    JsonAddStr(strBuffer, "userId", "Longstreet360");
    JsonAddStr(strBuffer, "userType", "Gamertag");
    JsonObjectEnd(strBuffer);
    JsonArrayEnd(strBuffer);
    JsonAddStr(strBuffer, "sender", "Striker");
    JsonAddStr(strBuffer, "senderPlatform", "null");
    JsonAddStr(strBuffer, "sent", "2011-10-13T16:40:58.1890842-07:00"); // AddDate()?
    JsonArrayStart(strBuffer, "targetPlatforms");
    JsonAddStr(strBuffer, NULL, "MP3");
    JsonArrayEnd(strBuffer);
    JsonAddInt(strBuffer, "title", 1297287259);
    JsonObjectEnd(strBuffer);
    pBuffer = JsonFinish(strBuffer);
    ZPrintf("encoded Json:\n------------\n%s\n-----------\n", pBuffer);
}

static void _ArrayTests(void)
{
    int32_t iElement, iValue;
    uint16_t aJsonParseBuf[512];
    char strTemp[16];

    JsonParse(aJsonParseBuf, sizeof(aJsonParseBuf)/sizeof(aJsonParseBuf[0]), _pSimpleArrayStringTest, (int32_t)strlen(_pSimpleArrayStringTest));
    for (iElement = 0; iElement < 4; iElement += 1)
    {
        JsonGetString(JsonFind2(aJsonParseBuf, NULL, "[", iElement), strTemp, sizeof(strTemp), "");
        ZPrintf("str[%d]=%s\n", iElement, strTemp);
    }

    JsonParse(aJsonParseBuf, sizeof(aJsonParseBuf)/sizeof(aJsonParseBuf[0]), _pSimpleArrayNumberTest, (int32_t)strlen(_pSimpleArrayNumberTest));
    for (iElement = 0; iElement < 5; iElement += 1)
    {
        iValue = (int32_t)JsonGetInteger(JsonFind2(aJsonParseBuf, NULL, "[", iElement), -1);
        ZPrintf("str[%d]=%d\n", iElement, iValue);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdJson

    \Description
        Test the Json formatter and parser

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 12/11/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdJson(ZContext *argz, int32_t argc, char *argv[])
{
    // check for a file argument... if we have one, load and parse it
    if (argc == 2)
    {
        uint16_t *pJsonParseBuf;
        int32_t iFileSize;
        char *pFileData;

        if ((pFileData = ZFileLoad(argv[1], &iFileSize, FALSE)) != NULL)
        {
            if ((pJsonParseBuf = JsonParse2(pFileData, -1, 0, 0, NULL)) != NULL)
            {
                //
                // insert custom parsing code here
                //
            }
            else
            {
                ZPrintf("%s: could not parse buffer\n", argv[0]);
            }

            free(pFileData);
        }
        else
        {
            ZPrintf("%s: could not open file '%s'\n", argv[0], argv[1]);
        }
    }
    else
    {
        _SimpleDecode();
        _SimpleEncode();
        // test parsing some simple arrays
        _ArrayTests();
    }

    //$$todo - more tests
    return(0);
}


