/*H********************************************************************************/
/*!
    \File lang.c

    \Description
        Test LobbyLang macros

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/03/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtylang.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "libsample/zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CmdLangTest

    \Description
        Test LobbyLang macros

    \Input ZContext     *argz   - environment
    \Input int32_t          argc    - standard number of arguments
    \Input char         **argv  - standard arg list
    
    \Output standard return value
*/
/********************************************************************************F*/
int32_t CmdLang(ZContext *argz, int32_t argc, char **argv)
{
    // check usage
    if (argc < 1)
    {
        ZPrintf("   test all lobbylang macros and display output\n");
        ZPrintf("   usage: %s \n", argv[0]);
        return(0);
    }
    else
    {
        uint32_t uToken;
        uint32_t uCurrency;
        uint16_t uLanguage;
        uint16_t uCountry;
        uint16_t uSymbols;
        uint16_t uTemp;
        char strCurrency[4];
        char strLang[3];
        char strCountry[3];
        char strToken[5];

        uLanguage = LOBBYAPI_LANGUAGE_GERMAN;
        uCountry = LOBBYAPI_COUNTRY_GERMANY;
        uToken = LOBBYAPI_LocalizerTokenCreate(uLanguage, uCountry);
        ZPrintf("token for germany and german: %c%c%c%c\n", LOBBYAPI_LocalizerTokenPrintCharArray(uToken));
        uCountry = LOBBYAPI_LocalizerTokenGetCountry(uToken);
        uLanguage = LOBBYAPI_LocalizerTokenGetLanguage(uToken);
        uSymbols = LOBBYAPI_LocalizerTokenGetShortFromString("*^");
        uTemp = LOBBYAPI_LocalizerTokenShortToUpper(uLanguage);
        ZPrintf("language [%c%c=0x%X] toupper [%c%c=0x%X]\n", 
            (uLanguage>>8)&0xFF, uLanguage&0xFF, uLanguage, 
            (uTemp>>8)&0xFF, uTemp&0xFF, uTemp);
        uTemp = LOBBYAPI_LocalizerTokenShortToLower(uCountry);
        ZPrintf("country [%c%c=0x%X] tolower [%c%c=0x%X]\n", 
            (uCountry>>8)&0xFF, uCountry&0xFF, uCountry, 
            (uTemp>>8)&0xFF, uTemp&0xFF, uTemp);
        ZPrintf("symbols (should not change) 0x%X toupper 0x%X\n",uSymbols, LOBBYAPI_LocalizerTokenShortToUpper(uSymbols));
        LOBBYAPI_LocalizerTokenSetCountry(uToken, LOBBYAPI_COUNTRY_UNITED_STATES);
        ZPrintf("token change to USA: %c%c%c%c\n", LOBBYAPI_LocalizerTokenPrintCharArray(uToken));
        LOBBYAPI_LocalizerTokenSetLanguage(uToken, LOBBYAPI_LANGUAGE_ENGLISH);
        ZPrintf("token change to USA and ENGLISH: %c%c%c%c\n", LOBBYAPI_LocalizerTokenPrintCharArray(uToken));
        LOBBYAPI_LocalizerTokenCreateCountryString(strCountry, uToken);
        ZPrintf("Country string [%s]\n", strCountry);
        LOBBYAPI_LocalizerTokenCreateLanguageString(strLang, uToken);
        ZPrintf("Language string [%s]\n", strLang);
        LOBBYAPI_LocalizerTokenCreateLocalityString(strToken, uToken);
        ZPrintf("Locality string [%s]\n", strToken);
        ZPrintf("int16_t from string 'Aa': 0x%X\n", LOBBYAPI_LocalizerTokenGetShortFromString("Aa"));
        uToken = LOBBYAPI_LocalizerTokenCreateFromStrings("FR", "FR");
        ZPrintf("create from strings: %c%c%c%c\n", LOBBYAPI_LocalizerTokenPrintCharArray(uToken));
        sprintf(strToken, "frCA");
        uToken = LOBBYAPI_LocalizerTokenCreateFromString(strToken);
        ZPrintf("create from string: %s --> %c%c%c%c\n", strToken, LOBBYAPI_LocalizerTokenPrintCharArray(uToken));

        // currency
        uCurrency = LOBBYAPI_CURRENCY_EURO;
        LOBBYAPI_CreateCurrencyString(strCurrency, uCurrency);
        ZPrintf("Currency string [%s]\n", strCurrency);

        uCurrency = LOBBYAPI_CURRENCY_UNITED_STATES_DOLLAR;
        LOBBYAPI_CreateCurrencyString(strCurrency, uCurrency);
        ZPrintf("Currency string [%s]\n", strCurrency);

        uCurrency = LOBBYAPI_CURRENCY_CANADIAN_DOLLAR;
        LOBBYAPI_CreateCurrencyString(strCurrency, uCurrency);
        ZPrintf("Currency string [%s]\n", strCurrency);

//        > langtest
//        token for germany and german: deDE
//        language [de=0x6465] toupper [DE=0x4445]
//        country [DE=0x4445] tolower [de=0x6465]
//        symbols (should not change) 0x2A5E toupper 0x2A5E
//        token change to USA: deUS
//        token change to USA and ENGLISH: enUS
//        Country string [US]
//        Language string [en]
//        Locality string [enUS]
//        int16_t from string 'Aa': 0x4161
//        create from strings: frFR
//        create from string: frCA --> frCA

        uToken = NetConnStatus('locl', 0, NULL, 0);
        ZPrintf("Current locl: %c%c%c%c\n", LOBBYAPI_LocalizerTokenPrintCharArray(uToken));

    }
    return(0);
}

