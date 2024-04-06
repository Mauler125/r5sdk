/*H********************************************************************************/
/*!
    \File hpack.c

    \Description
        Test the HPACK encoder and decoder.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/util/hpack.h"

#include "testermodules.h"

/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdHpackDecodeRequest

    \Description
        Test cases from the RFC Appendix C.3
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.3

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackDecodeRequest(void)
{
    HpackRefT *pRef;
    char strHeader[1024*1];
    uint8_t aFirstRequest[] = { 0x82, 0x86, 0x84, 0x41, 0x0f, 0x77, 0x77, 0x77, 0x2e,
                                0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63,
                                0x6f, 0x6d };
    uint8_t aSecondRequest[] = { 0x82, 0x86, 0x84, 0xbe, 0x58, 0x08, 0x6e, 0x6f, 0x2d,
                                 0x63, 0x61, 0x63, 0x68, 0x65 };
    uint8_t aThirdRequest[] = { 0x82, 0x87, 0x85, 0xbf, 0x40, 0x0a, 0x63, 0x75, 0x73,
                                0x74, 0x6f, 0x6d, 0x2d, 0x6b, 0x65, 0x79, 0x0c, 0x63,
                                0x75, 0x73, 0x74, 0x6f, 0x6d, 0x2d, 0x76, 0x61, 0x6c,
                                0x75, 0x65 };

    if ((pRef = HpackCreate(4096, TRUE)) == NULL)
    {
        return;
    }

    HpackDecode(pRef, aFirstRequest, sizeof(aFirstRequest), strHeader, sizeof(strHeader));
    ZPrintf("first request\n%s\n", strHeader);
    HpackDecode(pRef, aSecondRequest, sizeof(aSecondRequest), strHeader, sizeof(strHeader));
    ZPrintf("second request\n%s\n", strHeader);
    HpackDecode(pRef, aThirdRequest, sizeof(aThirdRequest), strHeader, sizeof(strHeader));
    ZPrintf("third request\n%s\n", strHeader);

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackDecodeResponse

    \Description
        Test cases from the RFC Appendix C.5
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.5

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackDecodeResponse(void)
{
    HpackRefT *pRef;
    char strHeader[1024*1];
    uint8_t aFirstResponse[] = { 0x48, 0x03, 0x33, 0x30, 0x32, 0x58, 0x07, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x61, 0x1d,
                                 0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x32, 0x31, 0x20, 0x4f, 0x63, 0x74, 0x20, 0x32, 0x30, 0x31, 0x33,
                                 0x20, 0x32, 0x30, 0x3a, 0x31, 0x33, 0x3a, 0x32, 0x31, 0x20, 0x47, 0x4d, 0x54, 0x6e, 0x17, 0x68,
                                 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70,
                                 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    uint8_t aSecondResponse[] = { 0x48, 0x03, 0x33, 0x30, 0x37, 0xc1, 0xc0, 0xbf };
    uint8_t aThirdResponse[] = { 0x88, 0xc1, 0x61, 0x1d, 0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x32, 0x31, 0x20, 0x4f, 0x63, 0x74, 0x20,
                                 0x32, 0x30, 0x31, 0x33, 0x20, 0x32, 0x30, 0x3a, 0x31, 0x33, 0x3a, 0x32, 0x32, 0x20, 0x47, 0x4d,
                                 0x54, 0xc0, 0x5a, 0x04, 0x67, 0x7a, 0x69, 0x70, 0x77, 0x38, 0x66, 0x6f, 0x6f, 0x3d, 0x41, 0x53,
                                 0x44, 0x4a, 0x4b, 0x48, 0x51, 0x4b, 0x42, 0x5a, 0x58, 0x4f, 0x51, 0x57, 0x45, 0x4f, 0x50, 0x49,
                                 0x55, 0x41, 0x58, 0x51, 0x57, 0x45, 0x4f, 0x49, 0x55, 0x3b, 0x20, 0x6d, 0x61, 0x78, 0x2d, 0x61,
                                 0x67, 0x65, 0x3d, 0x33, 0x36, 0x30, 0x30, 0x3b, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
                                 0x3d, 0x31 };

    if ((pRef = HpackCreate(256, TRUE)) == NULL)
    {
        return;
    }

    HpackDecode(pRef, aFirstResponse, sizeof(aFirstResponse), strHeader, sizeof(strHeader));
    ZPrintf("first response\n%s\n", strHeader);
    HpackDecode(pRef, aSecondResponse, sizeof(aSecondResponse), strHeader, sizeof(strHeader));
    ZPrintf("second response\n%s\n", strHeader);
    HpackDecode(pRef, aThirdResponse, sizeof(aThirdResponse), strHeader, sizeof(strHeader));
    ZPrintf("third response\n%s\n", strHeader);

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackDecodeRequestHuffman

    \Description
        Test cases from the RFC Appendix C.4
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.4

    \Version 10/18/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackDecodeRequestHuffman(void)
{
    HpackRefT *pRef;
    char strHeader[1024*1];
    uint8_t aFirstRequest[] = { 0x82, 0x86, 0x84, 0x41, 0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff };
    uint8_t aSecondRequest[] = { 0x82, 0x86, 0x84, 0xbe, 0x58, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf };
    uint8_t aThirdRequest[] = { 0x82, 0x87, 0x85, 0xbf, 0x40, 0x88, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9, 0x7d, 0x7f, 0x89, 0x25,
                                0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf };

    if ((pRef = HpackCreate(4096, TRUE)) == NULL)
    {
        return;
    }

    HpackDecode(pRef, aFirstRequest, sizeof(aFirstRequest), strHeader, sizeof(strHeader));
    ZPrintf("first request (huffman)\n%s\n", strHeader);
    HpackDecode(pRef, aSecondRequest, sizeof(aSecondRequest), strHeader, sizeof(strHeader));
    ZPrintf("second request (huffman)\n%s\n", strHeader);
    HpackDecode(pRef, aThirdRequest, sizeof(aThirdRequest), strHeader, sizeof(strHeader));
    ZPrintf("third request (huffman)\n%s\n", strHeader);

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackDecodeResponseHuffman

    \Description
        Test cases from the RFC Appendix C.6
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.6

    \Version 10/18/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackDecodeResponseHuffman(void)
{
    HpackRefT *pRef;
    char strHeader[1024*1];
    uint8_t aFirstResponse[] = { 0x48, 0x82, 0x64, 0x02, 0x58, 0x85, 0xae, 0xc3, 0x77, 0x1a, 0x4b, 0x61, 0x96, 0xd0, 0x7a, 0xbe,
                                 0x94, 0x10, 0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95, 0x04, 0x0b, 0x81, 0x66, 0xe0, 0x82, 0xa6,
                                 0x2d, 0x1b, 0xff, 0x6e, 0x91, 0x9d, 0x29, 0xad, 0x17, 0x18, 0x63, 0xc7, 0x8f, 0x0b, 0x97, 0xc8,
                                 0xe9, 0xae, 0x82, 0xae, 0x43, 0xd3 };
    uint8_t aSecondResponse[] = { 0x48, 0x83, 0x64, 0x0e, 0xff, 0xc1, 0xc0, 0xbf };
    uint8_t aThirdResponse[] = { 0x88, 0xc1, 0x61, 0x96, 0xd0, 0x7a, 0xbe, 0x94, 0x10, 0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95,
                                 0x04, 0x0b, 0x81, 0x66, 0xe0, 0x84, 0xa6, 0x2d, 0x1b, 0xff, 0xc0, 0x5a, 0x83, 0x9b, 0xd9, 0xab,
                                 0x77, 0xad, 0x94, 0xe7, 0x82, 0x1d, 0xd7, 0xf2, 0xe6, 0xc7, 0xb3, 0x35, 0xdf, 0xdf, 0xcd, 0x5b,
                                 0x39, 0x60, 0xd5, 0xaf, 0x27, 0x08, 0x7f, 0x36, 0x72, 0xc1, 0xab, 0x27, 0x0f, 0xb5, 0x29, 0x1f,
                                 0x95, 0x87, 0x31, 0x60, 0x65, 0xc0, 0x03, 0xed, 0x4e, 0xe5, 0xb1, 0x06, 0x3d, 0x50, 0x07 };

    if ((pRef = HpackCreate(256, TRUE)) == NULL)
    {
        return;
    }

    HpackDecode(pRef, aFirstResponse, sizeof(aFirstResponse), strHeader, sizeof(strHeader));
    ZPrintf("first response (huffman)\n%s\n", strHeader);
    HpackDecode(pRef, aSecondResponse, sizeof(aSecondResponse), strHeader, sizeof(strHeader));
    ZPrintf("second response (huffman)\n%s\n", strHeader);
    HpackDecode(pRef, aThirdResponse, sizeof(aThirdResponse), strHeader, sizeof(strHeader));
    ZPrintf("third response (huffman)\n%s\n", strHeader);

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackEncodeRequest

    \Description
        Test cases from the RFC Appendix C.3 (encoding)
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.3

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackEncodeRequest(void)
{
    HpackRefT *pRef;
    uint8_t aOutput[1024] = { 0 };
    char strFirstRequest[] = ":method: GET\r\n:scheme: http\r\n:path: /\r\n:authority: www.example.com\r\n";
    uint8_t aFirstRequest[] = { 0x82, 0x86, 0x84, 0x41, 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
                                0x2e, 0x63, 0x6f, 0x6d };

    char strSecondRequest[] = ":method: GET\r\n:scheme: http\r\n:path: /\r\n:authority: www.example.com\r\ncache-control: no-cache\r\n";
    uint8_t aSecondRequest[] = { 0x82, 0x86, 0x84, 0xbe, 0x58, 0x08, 0x6e, 0x6f, 0x2d, 0x63, 0x61, 0x63, 0x68, 0x65 };

    char strThirdRequest[] = ":method: GET\r\n:scheme: https\r\n:path: /index.html\r\n:authority: www.example.com\r\ncustom-key: custom-value\r\n";
    uint8_t aThirdRequest[] = { 0x82, 0x87, 0x85, 0xbf, 0x40, 0x0a, 0x63, 0x75, 0x73,
        0x74, 0x6f, 0x6d, 0x2d, 0x6b, 0x65, 0x79, 0x0c, 0x63,
        0x75, 0x73, 0x74, 0x6f, 0x6d, 0x2d, 0x76, 0x61, 0x6c,
        0x75, 0x65 };

    if ((pRef = HpackCreate(4096, FALSE)) == NULL)
    {
        return;
    }

    HpackEncode(pRef, strFirstRequest, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aFirstRequest, sizeof(aFirstRequest)) == 0)
    {
        ZPrintf("successfully encoded first request\n");
    }
    HpackEncode(pRef, strSecondRequest, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aSecondRequest, sizeof(aSecondRequest)) == 0)
    {
        ZPrintf("successfully encoded second request\n");
    }
    HpackEncode(pRef, strThirdRequest, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aThirdRequest, sizeof(aThirdRequest)) == 0)
    {
        ZPrintf("successfully encoded third request\n");
    }

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackEncodeResponse

    \Description
        Test cases from the RFC Appendix C.5 (encode)
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.5

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackEncodeResponse(void)
{
    HpackRefT *pRef;
    uint8_t aOutput[1024] = { 0 };

    char strFirstResponse[] = ":status: 302\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:21 GMT\r\nlocation: https://www.example.com\r\n";
    uint8_t aFirstResponse[] = { 0x48, 0x03, 0x33, 0x30, 0x32, 0x58, 0x07, 0x70, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x61, 0x1d,
        0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x32, 0x31, 0x20, 0x4f, 0x63, 0x74, 0x20, 0x32, 0x30, 0x31, 0x33,
        0x20, 0x32, 0x30, 0x3a, 0x31, 0x33, 0x3a, 0x32, 0x31, 0x20, 0x47, 0x4d, 0x54, 0x6e, 0x17, 0x68,
        0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70,
        0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };

    char strSecondResponse[] = ":status: 307\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:21 GMT\r\nlocation: https://www.example.com\r\n";
    uint8_t aSecondResponse[] = { 0x48, 0x03, 0x33, 0x30, 0x37, 0xc1, 0xc0, 0xbf };

    char strThirdResponse[] = ":status: 200\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:22 GMT\r\nlocation: https://www.example.com\r\ncontent-encoding: gzip\r\nset-cookie: foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1\r\n";
    uint8_t aThirdResponse[] = { 0x88, 0xc1, 0x61, 0x1d, 0x4d, 0x6f, 0x6e, 0x2c, 0x20, 0x32, 0x31, 0x20, 0x4f, 0x63, 0x74, 0x20,
        0x32, 0x30, 0x31, 0x33, 0x20, 0x32, 0x30, 0x3a, 0x31, 0x33, 0x3a, 0x32, 0x32, 0x20, 0x47, 0x4d,
        0x54, 0xc0, 0x5a, 0x04, 0x67, 0x7a, 0x69, 0x70, 0x77, 0x38, 0x66, 0x6f, 0x6f, 0x3d, 0x41, 0x53,
        0x44, 0x4a, 0x4b, 0x48, 0x51, 0x4b, 0x42, 0x5a, 0x58, 0x4f, 0x51, 0x57, 0x45, 0x4f, 0x50, 0x49,
        0x55, 0x41, 0x58, 0x51, 0x57, 0x45, 0x4f, 0x49, 0x55, 0x3b, 0x20, 0x6d, 0x61, 0x78, 0x2d, 0x61,
        0x67, 0x65, 0x3d, 0x33, 0x36, 0x30, 0x30, 0x3b, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e,
        0x3d, 0x31 };

    if ((pRef = HpackCreate(4096, FALSE)) == NULL)
    {
        return;
    }

    HpackEncode(pRef, strFirstResponse, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aFirstResponse, sizeof(aFirstResponse)) == 0)
    {
        ZPrintf("successfully encoded first response\n");
    }
    HpackEncode(pRef, strSecondResponse, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aSecondResponse, sizeof(aSecondResponse)) == 0)
    {
        ZPrintf("successfully encoded second response\n");
    }
    HpackEncode(pRef, strThirdResponse, aOutput, sizeof(aOutput), FALSE);
    if (memcmp(aOutput, aThirdResponse, sizeof(aThirdResponse)) == 0)
    {
        ZPrintf("successfully encoded third response\n");
    }

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackEncodeRequestHuffman

    \Description
        Test cases from the RFC Appendix C.4 (encode)
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.4

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackEncodeRequestHuffman(void)
{
    HpackRefT *pRef;
    uint8_t aOutput[1024] = { 0 };
    char strFirstRequest[] = ":method: GET\r\n:scheme: http\r\n:path: /\r\n:authority: www.example.com\r\n";
    char strSecondRequest[] = ":method: GET\r\n:scheme: http\r\n:path: /\r\n:authority: www.example.com\r\ncache-control: no-cache\r\n";
    char strThirdRequest[] = ":method: GET\r\n:scheme: https\r\n:path: /index.html\r\n:authority: www.example.com\r\ncustom-key: custom-value\r\n";
    uint8_t aFirstRequest[] = { 0x82, 0x86, 0x84, 0x41, 0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff };
    uint8_t aSecondRequest[] = { 0x82, 0x86, 0x84, 0xbe, 0x58, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf };
    uint8_t aThirdRequest[] = { 0x82, 0x87, 0x85, 0xbf, 0x40, 0x88, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9, 0x7d, 0x7f, 0x89, 0x25,
        0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf };

    if ((pRef = HpackCreate(4096, FALSE)) == NULL)
    {
        return;
    }

    HpackEncode(pRef, strFirstRequest, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aFirstRequest, sizeof(aFirstRequest)) == 0)
    {
        ZPrintf("successfully encoded first request (huffman)\n");
    }
    HpackEncode(pRef, strSecondRequest, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aSecondRequest, sizeof(aSecondRequest)) == 0)
    {
        ZPrintf("successfully encoded second request (huffman)\n");
    }
    HpackEncode(pRef, strThirdRequest, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aThirdRequest, sizeof(aThirdRequest)) == 0)
    {
        ZPrintf("successfully encoded third request (huffman)\n");
    }

    HpackDestroy(pRef);
}

/*F********************************************************************************/
/*!
    \Function _CmdHpackEncodeResponseHuffman

    \Description
        Test cases from the RFC Appendix C.6 (encode)
        Link: https://tools.ietf.org/html/rfc7541#appendix-C.6

    \Version 10/21/2016 (eesponda)
*/
/********************************************************************************F*/
static void _CmdHpackEncodeResponseHuffman(void)
{
    HpackRefT *pRef;
    uint8_t aOutput[1024] = { 0 };

    char strFirstResponse[] = ":status: 302\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:21 GMT\r\nlocation: https://www.example.com\r\n";
    char strSecondResponse[] = ":status: 307\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:21 GMT\r\nlocation: https://www.example.com\r\n";
    char strThirdResponse[] = ":status: 200\r\ncache-control: private\r\ndate: Mon, 21 Oct 2013 20:13:22 GMT\r\nlocation: https://www.example.com\r\ncontent-encoding: gzip\r\nset-cookie: foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1\r\n";
    uint8_t aFirstResponse[] = { 0x48, 0x82, 0x64, 0x02, 0x58, 0x85, 0xae, 0xc3, 0x77, 0x1a, 0x4b, 0x61, 0x96, 0xd0, 0x7a, 0xbe,
        0x94, 0x10, 0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95, 0x04, 0x0b, 0x81, 0x66, 0xe0, 0x82, 0xa6,
        0x2d, 0x1b, 0xff, 0x6e, 0x91, 0x9d, 0x29, 0xad, 0x17, 0x18, 0x63, 0xc7, 0x8f, 0x0b, 0x97, 0xc8,
        0xe9, 0xae, 0x82, 0xae, 0x43, 0xd3 };
    uint8_t aSecondResponse[] = { 0x48, 0x83, 0x64, 0x0e, 0xff, 0xc1, 0xc0, 0xbf };
    uint8_t aThirdResponse[] = { 0x88, 0xc1, 0x61, 0x96, 0xd0, 0x7a, 0xbe, 0x94, 0x10, 0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95,
        0x04, 0x0b, 0x81, 0x66, 0xe0, 0x84, 0xa6, 0x2d, 0x1b, 0xff, 0xc0, 0x5a, 0x83, 0x9b, 0xd9, 0xab,
        0x77, 0xad, 0x94, 0xe7, 0x82, 0x1d, 0xd7, 0xf2, 0xe6, 0xc7, 0xb3, 0x35, 0xdf, 0xdf, 0xcd, 0x5b,
        0x39, 0x60, 0xd5, 0xaf, 0x27, 0x08, 0x7f, 0x36, 0x72, 0xc1, 0xab, 0x27, 0x0f, 0xb5, 0x29, 0x1f,
        0x95, 0x87, 0x31, 0x60, 0x65, 0xc0, 0x03, 0xed, 0x4e, 0xe5, 0xb1, 0x06, 0x3d, 0x50, 0x07 };

    if ((pRef = HpackCreate(4096, FALSE)) == NULL)
    {
        return;
    }

    HpackEncode(pRef, strFirstResponse, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aFirstResponse, sizeof(aFirstResponse)) == 0)
    {
        ZPrintf("successfully encoded first response (huffman)\n");
    }

    HpackEncode(pRef, strSecondResponse, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aSecondResponse, sizeof(aSecondResponse)) == 0)
    {
        ZPrintf("successfully encoded second response (huffman)\n");
    }
    HpackEncode(pRef, strThirdResponse, aOutput, sizeof(aOutput), TRUE);
    if (memcmp(aOutput, aThirdResponse, sizeof(aThirdResponse)) == 0)
    {
        ZPrintf("successfully encoded third response (huffman)\n");
    }

    HpackDestroy(pRef);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHpack

    \Description
        Test the HPACK encoder and decoder.

    \Input *pArgz   - environment
    \Input iArgc    - standard number of arguments
    \Input *pArgv[] - standard arg list

    \Output standard return value

    \Version 10/07/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t CmdHpack(ZContext *pArgz, int32_t iArgc, char *pArgcv[])
{
    _CmdHpackDecodeRequest();
    _CmdHpackDecodeResponse();
    _CmdHpackDecodeRequestHuffman();
    _CmdHpackDecodeResponseHuffman();
    _CmdHpackEncodeRequest();
    _CmdHpackEncodeResponse();
    _CmdHpackEncodeRequestHuffman();
    _CmdHpackEncodeResponseHuffman();
    return(0);
}
