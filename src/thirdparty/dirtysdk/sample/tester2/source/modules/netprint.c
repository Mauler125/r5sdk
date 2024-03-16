/*H********************************************************************************/
/*!
    \File netprint.c

    \Description
        Test ds_vsnprintf() for compliance with standard routines.

    \Copyright
        Copyright (c) 2009-2010 Electronic Arts Inc.

    \Version 10/28/2009 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "libsample/zlib.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

// Variables

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _NetFormatAddr6

    \Description
        Initialize a struct sockaddr_in6 given address and port

    \Input *pAddr6      - address to fill in 
    \Input *pWords      - 128 bit address
    \Input *uPort       - port

    \Version 09/01/2017 (jbrookes)
*/
/********************************************************************************F*/
#if !defined(DIRTYCODE_NX)
static void _NetFormatAddr6(struct sockaddr_in6 *pAddr6, const uint16_t *pWords, uint16_t uPort)
{
    int32_t iWord;
    ds_memclr(pAddr6, sizeof(*pAddr6));
    pAddr6->sin6_family = AF_INET6;
    pAddr6->sin6_port = SocketNtohs(uPort);
    pAddr6->sin6_flowinfo = 0;
    for (iWord = 0; iWord < 8; iWord += 1)
    {
        pAddr6->sin6_addr.s6_addr[(iWord*2)+0] = (uint8_t)(pWords[iWord]>>8);
        pAddr6->sin6_addr.s6_addr[(iWord*2)+1] = (uint8_t)(pWords[iWord]&0xff);
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _NetPrintStrCmp

    \Description
        Compare two strings; if they are the same, print one out, otherwise
        print both out.

    \Input *pStrCtrl    - pointer to "control" string (what we are expecting)
    \Input *pStrTest    - pointer to "test" string (what we actually produced)
    \Input *pStrType    - pointer to type field, to use in displaying the result

    \Output
        int32_t         - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStrCmp(const char *pStrCtrl, const char *pStrTest, const char *pStrType)
{
    int32_t iResult;
    if (strcmp(pStrCtrl, pStrTest) != 0)
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s)\n", pStrType, pStrCtrl, pStrTest);
        iResult = 1;
    }
    else
    {
        ZPrintf("netprint: [%s] \"%s\"\n", pStrType, pStrTest);
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintMemCmp

    \Description
        Compare two buffers; if they are the same, print one out, otherwise
        print both out.

    \Input *pCtrl    - pointer to "control" buffer (what we are expecting)
    \Input *pTest    - pointer to "test" buffer (what we actually produced)
    \Input iLen      - buffer length
    \Input *pType    - pointer to type field, to use in displaying the result

    \Output
        int32_t         - 0=same, 1=different

    \Version 09/01/2017 (jbrookes)
*/
/********************************************************************************F*/
#ifndef DIRTYCODE_NX
static int32_t _NetPrintMemCmp(const uint8_t *pCtrl, int32_t iCtrlLen, const uint8_t *pTest, int32_t iTestLen, const char *pStrType)
{
    int32_t iResult = 1;
    if (iCtrlLen != iTestLen)
    {
        ZPrintf("netprint: [%s] ctrllen(%d) != testlen(%d)\n", pStrType, iCtrlLen, iTestLen);
    }
    else if (memcmp(pCtrl, pTest, iCtrlLen) != 0)
    {
        ZPrintf("netprint: [%s] ctrl != test\n", pStrType);
        NetPrintMem(pCtrl, iCtrlLen, "ctrl");
        NetPrintMem(pTest, iCtrlLen, "test");
    }
    else
    {
        ZPrintf("netprint: [%s] ctrl=test\n", pStrType);
        iResult = 0;
    }
    return(iResult);
}
#endif

/*F********************************************************************************/
/*!
    \Function _NetPrintStrIntCmp

    \Description
        Compare two strings AND two results; if they are the same, print one out,
        otherwise print both out.

    \Input *pStrCtrl    - pointer to "control" string (what we are expecting)
    \Input *_pStrTest   - pointer to "test" string (what we actually produced)
    \Input iCtrlRslt    - expected result
    \Input iTestRslt    - actual result
    \Input iBufferLimit - size of buffer we were writing into
    \Input *pStrType    - pointer to type field, to use in displaying the result

    \Output
        int32_t         - 0=same, 1=different

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStrIntCmp(const char *pStrCtrl, const char *_pStrTest, int32_t iCtrlRslt, int32_t iTestRslt, int32_t iBufferLimit, const char *pStrType)
{
    int32_t iResult, iStrCmp;
    const char *pStrTest;

    // if we have a zero-sized buffer, point to an empty string so we can compare/print it safely
    pStrTest = (iBufferLimit > 0) ? _pStrTest : "";

    // compare the strings
    iStrCmp = strcmp(pStrCtrl, pStrTest);

    if ((iStrCmp != 0) && (iCtrlRslt != iTestRslt))
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s) and ctrl(%d) != test(%d)\n", pStrType, pStrCtrl, pStrTest, iCtrlRslt, iTestRslt);
        iResult = 1;
    }
    else if (iStrCmp != 0)
    {
        ZPrintf("netprint: [%s] ctrl(%s) != test(%s)\n", pStrType, pStrCtrl, pStrTest);
        iResult = 1;
    }
    else if (iCtrlRslt != iTestRslt)
    {
        ZPrintf("netprint: [%s] ctrl(%d) != test(%d)\n", pStrType, iCtrlRslt, iTestRslt);
        iResult = 1;
    }
    else
    {
        if (iTestRslt > 0)
        {
            ZPrintf("netprint: [%s] \"%s\" (%d)\n", pStrType, pStrTest, iTestRslt);
        }
        else
        {
            ZPrintf("netprint: [%s] "" (%d)\n", pStrType, iTestRslt);
        }
        iResult = 0;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintFloat

    \Description
        Print a floating-point value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input fValue   - float to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintFloat(const char *pFmt, double fValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, fValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, fValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "flt"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintInt

    \Description
        Print an integer value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input iValue   - integer to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintInt(const char *pFmt, int32_t iValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, iValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, iValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "int"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintLongInt

    \Description
        Print a 64-bit integer value with both ds_snzprintf and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input iValue   - 64-bit integer to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintLongInt(const char *pFmt, int64_t iValue)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, iValue);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, iValue);

    return(_NetPrintStrCmp(strCtrl, strTest, "lng"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintStr

    \Description
        Print a string with both ds_snzprintf() and platform sprintf(),
        and compare the results.  Flag a warning if they are different.

    \Input *pFmt    - format string
    \Input *pStr    - string to print

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintStr(const char *pFmt, const char *pStr)
{
    char strCtrl[256], strTest[256];

    sprintf(strCtrl, pFmt, pStr);
    ds_snzprintf(strTest, sizeof(strTest), pFmt, pStr);

    return(_NetPrintStrCmp(strCtrl, strTest, "str"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintPregen

    \Description
        Print formatted output with ds_vsnprintf() and compare to a pre-generated
        (static) string.  Flag a warning if they are different.

    \Input *pPreGen - pointer to pre-generated string (what we expect)
    \Input *pFmt    - format specifier
    \Input ...      - variable argument list

    \Output
        int32_t     - 0=same, 1=different

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintPregen(const char *pPreGen, const char *pFmt, ...)
{
    char strTest[1024];
    va_list Args;

    // format the output
    va_start(Args, pFmt);
    ds_vsnprintf(strTest, sizeof(strTest), pFmt, Args);
    va_end(Args);

    return(_NetPrintStrCmp(pPreGen, strTest, "pre"));
}

/*F********************************************************************************/
/*!
    \Function _NetPrintOverflow

    \Description
        Print formatted output with ds_vsnprintf() and compare to a pre-generated
        (static) string.  Flag a warning if they are different.

    \Input iBufferLimit     - size we want to limit buffer to
    \Input *pPreGen         - pre-generated string to compare formatted result to
    \Input iExpectedResult  - expected result code from ds_vsnzprintf()
    \Input *pFmt            - format specifier
    \Input ...              - variable argument list

    \Output
        int32_t             - 0=same, 1=different

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintOverflow(int32_t iBufferLimit, const char *pPreGen, int32_t iExpectedResult, const char *pFmt, ...)
{
    char strTest[128];
    int32_t iResult, iStrCmp, iCheck, iMemStomp;
    char cMemChar = 0xcc;
    va_list Args;

    // pre-initialize array
    ds_memset(strTest, cMemChar, sizeof(strTest));

    // format the output
    va_start(Args, pFmt);
    iResult = ds_vsnzprintf(strTest, iBufferLimit, pFmt, Args);
    va_end(Args);

    // make sure we didn't write outside our bounds
    for (iCheck = iBufferLimit, iMemStomp = 0; iCheck < (signed)sizeof(strTest); iCheck += 1)
    {
        if (strTest[iCheck] != cMemChar)
        {
            iMemStomp += 1;
        }
    }

    // did the test succeed or fail?
    iStrCmp = _NetPrintStrIntCmp(pPreGen, strTest, iExpectedResult, iResult, iBufferLimit, "ovr");
    if ((iStrCmp != 0) || (iMemStomp != 0))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestFlts

    \Description
        Execute a series of floating-point printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestFlts(void)
{
    int32_t iResult = 0;
    ZPrintf("netprint: Floating-point comparative tests\n");
    iResult += _NetPrintFloat("%2.0f", 10.0f);
    iResult += _NetPrintFloat("%f", pow(2,52));
    iResult += _NetPrintFloat("%f", pow(2,53));
    iResult += _NetPrintFloat("%.15f", 0.00000000000000000000099f); // we don't test .16 here; ms printf doesn't round at 16+ digits
    iResult += _NetPrintFloat("%.15f", 0.0099f);
    iResult += _NetPrintFloat("%.15f", 0.0000000099f);
    iResult += _NetPrintFloat("%f", 1.99f);
    iResult += _NetPrintFloat("%f", -1.99);
    iResult += _NetPrintFloat("%f", 1.0f);
    iResult += _NetPrintFloat("%f", 0.75f);
    iResult += _NetPrintFloat("%2.2f", 1.0);
    iResult += _NetPrintFloat("%+2.2f", 1.0);
    iResult += _NetPrintFloat("%f", -1.99);
    iResult += _NetPrintFloat("%.2f", 9.99);
    iResult += _NetPrintFloat("%.2f", 9.999);
    iResult += _NetPrintFloat("%.2f", -1.999);
    iResult += _NetPrintFloat("%.2f", 0.1);
    iResult += _NetPrintFloat("%.15f", 3.1415926535897932384626433832795);

    /* this section is for stuff that is not compatible with sprintf() or
       is not compatible with the single-param _NetPrintFloat().  For these
       tests, we compare against a pre-generated string to make sure our
       output is consistently what we expect across all platforms. */

    // make sure all fp selectors result in %f behavior
    iResult += _NetPrintPregen("%e 1.000000 %E 1.000000", "%%e %e %%E %E", 1.0f, 1.0f);
    iResult += _NetPrintPregen("%g 1.000000 %G 1.000000", "%%g %g %%G %G", 1.0f, 1.0f);
    iResult += _NetPrintPregen("%F 1.000000", "%%F %F", 1.0f);
    // test variable width with fp
    iResult += _NetPrintPregen(" 1", "%2.*f", 2, 1.0f);
    // test a really large number, but less than our maximum
    iResult += _NetPrintPregen("9223372036854775808.000000", "%f", pow(2,63));
    // test a really large number, greater than our max
    iResult += _NetPrintPregen("0.(BIG)", "%f", pow(2,64));

    // floating-point test summary
    ZPrintf("netprint: %d floating-point test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestInts

    \Description
        Execute a series of integer printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestInts(void)
{
    int32_t iResult = 0;
    const int32_t iBits = (signed)sizeof(int32_t)*8;
    const int32_t iMaxInt = (1 << (iBits - 1)) + 1;
    int32_t iInt0 = 8;

    ZPrintf("netprint: Integer tests\n");

    iResult += _NetPrintInt("%d = 8", iInt0);
    iResult += _NetPrintInt("%+d = +8", iInt0);
    iResult += _NetPrintInt("%d = -maxint", iMaxInt);
    iResult += _NetPrintInt("char %c = 'a'", 'a');
    iResult += _NetPrintInt("hex %x = ff", 0xff);
    iResult += _NetPrintInt("hex %02x = 00", 0);
    iResult += _NetPrintInt("oct %o = 10", 010);
    iResult += _NetPrintInt("oct %03o = 010", 8);
    iResult += _NetPrintLongInt("%llu", 0x900f123412341234ull);
    iResult += _NetPrintLongInt("0x%llx", 0x900f123412341234ull);
#if defined(DIRTYCODE_PC)
    iResult += _NetPrintLongInt("%I64d", 0x900f123412341234ull);
#endif
#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS)
    iResult += _NetPrintLongInt("%qd", 0x900f123412341234ull);
#endif
    iResult += _NetPrintPregen("signed -5 = unsigned 4294967291 = hex fffffffb", "signed %d = unsigned %u = hex %x", -5, -5, -5);
    iResult += _NetPrintPregen("4294967286,-10", "%u,%d", -10, -10);
    iResult += _NetPrintPregen("0,10,20,30,100,200,1000", "%d,%d,%d,%d,%d,%d,%d",  0, 10, 20, 30, 100, 200, 1000);
    iResult += _NetPrintPregen("0,-10,-20,-30,-100,-200,-1000", "%d,%d,%d,%d,%d,%d,%d",  0, -10, -20, -30, -100, -200, -1000);

    ZPrintf("netprint: %d integer test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestPtrs

    \Description
        Execute a series of pointer printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestPtrs(void)
{
    int32_t iResult = 0;
    char *pStr = "string test", *pNul = NULL;
    char strTemp[128];

    ZPrintf("netprint: Pointer tests\n");
#if DIRTYCODE_64BITPTR
    _NetPrintPregen("p=$123456789abcdef0", "p=%p", (void *)0x123456789abcdef0);

#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_PS4)
    sprintf(strTemp, "p=$%016lx, null p=(null)", (uintptr_t)pStr);
#else
    sprintf(strTemp, "p=$%016llx, null p=(null)", (uintptr_t)pStr);
#endif

#else
    _NetPrintPregen("p=$12345678", "p=%p", (void *)0x12345678);
    sprintf(strTemp, "p=$%08x, null p=(null)", (uint32_t)pStr);
#endif
    iResult += _NetPrintPregen(strTemp, "p=%p, null p=%p", pStr, pNul);

    ZPrintf("netprint: %d pointer test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestStrs

    \Description
        Execute a series of string printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestStrs(void)
{
    int32_t iResult = 0;
    char *pStr = "string test";
    wchar_t *pWideStr = L"wide string test";

    ZPrintf("netprint: String tests\n");
    iResult += _NetPrintStr("string test=%s", pStr);
#if defined(DIRTYCODE_PC)
    iResult += _NetPrintStr("wide string test=%S", (const char *)pWideStr);
#else
    iResult += _NetPrintStr("wide string test=%ls", (const char *)pWideStr);
#endif
    iResult += _NetPrintStr("string test=%s", NULL);

    ZPrintf("netprint: %d string test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestAlgn

    \Description
        Execute a series of string alignment formatting tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestAlgn(void)
{
    int32_t iResult = 0;

    ZPrintf("netprint: Alignment tests\n");

    iResult += _NetPrintStr(" left just: \"%-10s\"", "left");
    iResult += _NetPrintStr("right just: \"%10s\"", "right");
    iResult += _NetPrintInt(" 6: %04d zero padded", 6);
    iResult += _NetPrintInt(" 6: %-4d left just", 6);
    iResult += _NetPrintInt(" 6: %4d right just", 6);
    iResult += _NetPrintInt("-6: %04d zero padded", -6);
    iResult += _NetPrintInt("-6: %-4d left just", -6);
    iResult += _NetPrintInt("-6: %4d right just", -6);
    iResult += _NetPrintPregen(" 6: 0006 zero padded, variable-length", " 6: %0*d zero padded, variable-length", 4, 6);
    iResult += _NetPrintPregen(" 6: 6    left just, variable-length", " 6: %-*d left just, variable-length", 4, 6);

    iResult += _NetPrintPregen(" a: a        left just", " a: %-8c left just", 'a');
    iResult += _NetPrintPregen(" b:        b right just", " b: %8c right just", 'b');
    iResult += _NetPrintPregen(" c: c        left just, variable-length", " c: %-*c left just, variable-length", 8, 'c');
    iResult += _NetPrintPregen(" d:        d right just, variable-length", " d: %*c right just, variable-length", 8, 'd');

    ZPrintf("netprint: %d alignment test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestMisc

    \Description
        Execute a series of miscelleneous printing tests.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestMisc(void)
{
    int32_t iResult = 0;
    int32_t iInt1 = 0;
    char strTest[256], strTest2[128];

    ZPrintf("netprint: Misc tests\n");

    // test %n selector
    ds_snzprintf(strTest, sizeof(strTest), "%%n test %n", &iInt1);
    ds_snzprintf(strTest2, sizeof(strTest2), "(%d chars written)", iInt1);
    ds_strnzcat(strTest, strTest2, sizeof(strTest));
    iResult += _NetPrintStrCmp("%n test (8 chars written)", strTest, "msc");

    iResult += _NetPrintPregen("# test: 10", "# test: %#d", 10);
    iResult += _NetPrintPregen("1 1 1 1 1.000000 1 1 1", "%hd %hhd %ld %lld %f %zd %jd %td", 1, 1, 1l, 1ll, 1.0, 1, 1, 1);
    iResult += _NetPrintPregen("testing invalid trailing pct: ", "testing invalid trailing pct: %");
    iResult += _NetPrintPregen("testing valid trailing pct: %", "testing valid trailing pct: %%");

    ZPrintf("netprint: %d misc test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestAddr

    \Description
        Execute a series of address printing tests.

    \Output
        int32_t     - number of test failures

    \Version 08/28/2017 (jbrookes)
*/
/********************************************************************************F*/
#ifndef DIRTYCODE_NX
static int32_t _NetPrintTestAddr(void)
{
    uint32_t uAddr, uNumAddrs, uWord;
    struct sockaddr_in6 SockAddr6;
    char strAddr6[48];
    int32_t iResult = 0;
    uint16_t aWords[8];

    struct _NetAddr6
    {
        const uint16_t aWords[8];
        const char *pAddrText;
    };
    static const struct _NetAddr6 _aAddr6ToString[] =
    {
        { { 0x2001, 0x0db8, 0x0000, 0x0000, 0x0008, 0x0800, 0x200c, 0x417A }, "2001:db8::8:800:200c:417a" },            // a unicast address
        { { 0xff01, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101 }, "ff01::101" },                            // a multicast address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 }, "::1" },                                  // the loopback address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, "::" },                                   // the unspecified address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xc0a8, 0x0001 }, "::ffff:192.168.0.1" },                   // an ipv4-mapped ipv6 address
        { { 0x0064, 0xff9b, 0x0000, 0x0000, 0x0000, 0x0000, 0xc0a8, 0x0001 }, "64:ff9b::192.168.0.1" },                 // a nat64 address
        { { 0x0102, 0x0000, 0x0506, 0x0000, 0x0000, 0x0708, 0x090a, 0x0000 }, "102:0:506::708:90a:0" },                 // synthetic test
        { { 0x1234, 0x5678, 0x9abc, 0xdef0, 0x0000, 0x1234, 0x5678, 0x9abc }, "1234:5678:9abc:def0:0:1234:5678:9abc" }  // synthetic test
    };
    static const struct _NetAddr6 _aStringToAddr6[] =
    {
        { { 0x2001, 0x0db8, 0x0000, 0x0000, 0x0008, 0x0800, 0x200c, 0x417A }, "2001:DB8::8:800:200C:417A" },            // a unicast address
        { { 0xff01, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101 }, "FF01::101" },                            // a multicast address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001 }, "::1" },                                  // the loopback address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }, "::" },                                   // the unspecified address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xc0a8, 0x0001 }, "::ffff:c0a8:1" },                        // an ipv4-mapped ipv6 address
        { { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xc0a8, 0x0001 }, "::ffff:192.168.0.1" },                   // a mixed-notation ipv4-mapped ipv6 address
        { { 0x0064, 0xff9b, 0x0000, 0x0000, 0x0000, 0x0000, 0xc0a8, 0x0001 }, "64:ff9b::c0a8:1" },                      // a nat64 address
        { { 0x0064, 0xff9b, 0x0000, 0x0000, 0x0000, 0x0000, 0xc0a8, 0x9b18 }, "64:ff9b::192.168.155.24" },              // a mixed-notation nat64 address
        { { 0x0102, 0x0000, 0x0506, 0x0000, 0x0000, 0x0708, 0x090a, 0x0000 }, "102:0:506::708:90a:0" },                 // synthetic test
        { { 0x1234, 0x5678, 0x9abc, 0xdef0, 0x0000, 0x1234, 0x5678, 0x9abc }, "1234:5678:9abc:def0:0:1234:5678:9abc" }  // synthetic test
    };

    // test addr6 to string conversion
    uNumAddrs = sizeof(_aAddr6ToString) / sizeof(_aAddr6ToString[0]);
    for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        _NetFormatAddr6(&SockAddr6, _aAddr6ToString[uAddr].aWords, 0);
        SockaddrInGetAddrText((struct sockaddr *)&SockAddr6, strAddr6, sizeof(strAddr6));
        iResult += _NetPrintStrCmp(_aAddr6ToString[uAddr].pAddrText, strAddr6, "addr");
    }

    // test string to addr6 conversion
    uNumAddrs = sizeof(_aStringToAddr6)/sizeof(_aStringToAddr6[0]);
    for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        SockaddrInSetAddrText((struct sockaddr *)&SockAddr6, _aStringToAddr6[uAddr].pAddrText);
        for (uWord = 0; uWord < 8; uWord += 1)
        {
            aWords[uWord] = SocketNtohs(*(uint16_t *)(SockAddr6.sin6_addr.s6_addr+(uWord*2)));
        }
        iResult += _NetPrintMemCmp((const uint8_t *)_aStringToAddr6[uAddr].aWords, sizeof(_aStringToAddr6[uAddr].aWords), (const uint8_t *)aWords, sizeof(aWords), "addr");
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestCust

    \Description
        Execute a series of custom printing tests.  These are for selectors that
        are specific to the DirtySock platform string formatting functions.

    \Output
        int32_t     - number of test failures

    \Version 05/05/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestCust(void)
{
    int32_t iResult = 0;
    struct sockaddr SockAddr4;
    struct sockaddr_in6 SockAddr6, SockAddr6_2, SockAddr6_3;
    uint8_t aSin6Addr[16] = { 0x5a, 0x23, 0x01, 0x32, 0xff, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xde, 0x00, 0x02 };
    uint32_t uAddr;

    SockaddrInit(&SockAddr4, AF_INET);
    SockaddrInSetAddr(&SockAddr4, 0xC0A80001);
    SockaddrInSetPort(&SockAddr4, 3658);

    // create an IPv4-mapped IPv6 address
    ds_memclr(&SockAddr6, sizeof(SockAddr6));
    uAddr = SockaddrInGetAddr(&SockAddr4);
    SockAddr6.sin6_family = AF_INET6;
    SockAddr6.sin6_port = SocketNtohs(SockaddrInGetPort(&SockAddr4));
    SockAddr6.sin6_flowinfo = 0;
    SockAddr6.sin6_addr.s6_addr[10] = 0xff;
    SockAddr6.sin6_addr.s6_addr[11] = 0xff;
    SockAddr6.sin6_addr.s6_addr[12] = (uint8_t)(uAddr >> 24);
    SockAddr6.sin6_addr.s6_addr[13] = (uint8_t)(uAddr >> 16);
    SockAddr6.sin6_addr.s6_addr[14] = (uint8_t)(uAddr >> 8);
    SockAddr6.sin6_addr.s6_addr[15] = (uint8_t)(uAddr >> 0);

    // create a simulated full IPv6 address
    SockaddrInit6(&SockAddr6_2, AF_INET6);
    SockAddr6_2.sin6_port = SocketNtohs(3658);
    ds_memcpy(&SockAddr6_2.sin6_addr, aSin6Addr, sizeof(SockAddr6_2.sin6_addr));

    // create a NAT64 address
    ds_memcpy(&SockAddr6_3, &SockAddr6, sizeof(SockAddr6_3));
    SockAddr6_3.sin6_addr.s6_addr[1] = 0x64;
    SockAddr6_3.sin6_addr.s6_addr[2] = 0xff;
    SockAddr6_3.sin6_addr.s6_addr[3] = 0x9b;
    SockAddr6_3.sin6_addr.s6_addr[10] = 0x00;
    SockAddr6_3.sin6_addr.s6_addr[11] = 0x00;

    ZPrintf("netprint: Custom tests\n");

    iResult += _NetPrintPregen("addr=192.168.0.1", "addr=%a", SockaddrInGetAddr(&SockAddr4));
    iResult += _NetPrintPregen("addr=255.255.255.255", "addr=%a", 0xffffffff);
    iResult += _NetPrintPregen("addr=[192.168.0.1]:3658", "addr=%A", &SockAddr4);
    iResult += _NetPrintPregen("addr=[::ffff:192.168.0.1]:3658", "addr=%A", &SockAddr6);
    iResult += _NetPrintPregen("addr=[64:ff9b::192.168.0.1]:3658", "addr=%A", &SockAddr6_3);
    iResult += _NetPrintPregen("addr=[5a23:132:ff12::12de:2]:3658", "addr=%A", &SockAddr6_2);
    iResult += _NetPrintPregen("'dflt' = 'dflt'", "'dflt' = '%C'", 'dflt');
    iResult += _NetPrintPregen("'d*l*' = 'd*l*'", "'d*l*' = '%C'", ('d' << 24) | ('l' << 8));

    ZPrintf("netprint: %d custom test discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");

    return(iResult);
}
#endif

/*F********************************************************************************/
/*!
    \Function _NetPrintTestOver

    \Description
        Execute a series of printing tests exercising the overflow functionality.

    \Output
        int32_t     - number of test failures

    \Version 02/15/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestOver(void)
{
    const char strOver[] = "<<<overflow>>>";
    int32_t iResult0 = 0, iResult1 = 0;

    // test some overflow scenarios with ds_snzprintf2
    ZPrintf("netprint: Overflow tests\n");
    iResult1 += _NetPrintOverflow( 0, "",                     14,    "%s", strOver);
    iResult1 += _NetPrintOverflow( 1, "",                     14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(12, "<<<overflow",          14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(14, "<<<overflow>>",        14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(15, "<<<overflow>>>",       14,    "%s", strOver);
    iResult1 += _NetPrintOverflow(20, "      <<<overflow>>",  20,  "%20s", strOver);
    iResult1 += _NetPrintOverflow(20, "<<<overflow>>>     ",  20, "%-20s", strOver);
    iResult1 += _NetPrintOverflow(21, "      <<<overflow>>>", 20,  "%20s", strOver);
    iResult1 += _NetPrintOverflow(16, "-<<<overflow>>>",      16,  "-%s-", strOver);
    iResult1 += _NetPrintOverflow(17, "-<<<overflow>>>-",     16,  "-%s-", strOver);

    ZPrintf("netprint: %d overflow test discrepencies\n", iResult1);
    ZPrintf("netprint: ------------------------------------\n");
    return(iResult0+iResult1);
}

/*F********************************************************************************/
/*!
    \Function _NetPrintTestSpam

    \Description
        Execute a series of printing tests exercising the spam suppression

    \Output
        int32_t     - number of test failures

    \Version 04/05/2016 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetPrintTestSpam(void)
{
    int32_t iTest;

    // test suppression of multiple identical lines, with output forced by a non-identical line
    NetPrintf(("netprint: rate limit test #1 start\n"));
    for (iTest = 0; iTest < 32; iTest += 1)
    {
        NetPrintf(("netprint: testing rate limiter\n"));
    }
    NetPrintf(("netprint: rate limit test #1 finish\n"));

    // test suppression of multiple identical lines, with output forced by timeout
    NetPrintf(("netprint: rate limit test #2 start\n"));
    for (iTest = 0; iTest < 32; iTest += 1)
    {
        NetPrintf(("netprint: testing rate limiter\n"));
        NetConnSleep(30);
    }
    NetPrintf(("netprint: rate limit test #2 finish\n"));

    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdNetPrint

    \Description
        Test the ds_vsnprintf function

    \Input *argz    - environment
    \Input argc     - standard number of arguments
    \Input *argv[]  - standard arg list

    \Output
        int32_t     - standard return value

    \Version 10/28/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdNetPrint(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iResult = 0;

    ZPrintf("netprint: ------------------------------------\n");
    ZPrintf("netprint: Testing ds_snzprintf() vs sprintf()\n");
    ZPrintf("netprint: ------------------------------------\n");

    iResult += _NetPrintTestStrs();
    iResult += _NetPrintTestFlts();
    iResult += _NetPrintTestInts();
    iResult += _NetPrintTestPtrs();

    #ifndef DIRTYCODE_NX
    iResult += _NetPrintTestAddr();
    iResult += _NetPrintTestCust();
    #endif

    iResult += _NetPrintTestAlgn();
    iResult += _NetPrintTestMisc();
    iResult += _NetPrintTestOver();
    iResult += _NetPrintTestSpam();

    ZPrintf("netprint: Test results: %d total discrepencies\n", iResult);
    ZPrintf("netprint: ------------------------------------\n");

    return(0);
}


