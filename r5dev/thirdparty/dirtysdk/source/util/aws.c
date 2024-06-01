/*H********************************************************************************/
/*!
    \File aws.c

    \Description
        Implements AWS utility functions, including SigV4 signing and signed binary
        event reading and writing.

    \Notes
        References:
            Signature V4: https://docs.aws.amazon.com/en_us/general/latest/gr/signature-version-4.html
            SigV4 Signing: https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4_signing.html
            SigV4 Examples: https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4-signed-request-examples.html
            Event Stream Encoding: https://docs.aws.amazon.com/transcribe/latest/dg/streaming-format.html
 
    \Copyright
        Copyright 2018 Electronic Arts

    \Version 12/26/2018 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/proto/protohttputil.h"

#include "DirtySDK/util/aws.h"

/*** Defines **********************************************************************/

//! define this for debug output
#define AWS_DEBUG                   (DIRTYCODE_DEBUG && FALSE)

//! length of aws binary event prelude
#define AWS_EVENT_PRELUDE_SIZE      (12)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! AWS event header types
typedef enum
{
    AWS_BOOLEAN_TRUE = 0,
    AWS_BOOLEAN_FALSE,
    AWS_BYTE,
    AWS_SHORT,
    AWS_INTEGER,
    AWS_LONG,
    AWS_BYTE_ARRAY,
    AWS_STRING,
    AWS_TIMESTAMP,
    AWS_UUID
} AWSHeaderValueE;

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _AWSHashSha2

    \Description
        Calculate SHA2 hash of specified data

    \Input *pHashBuf    - [out] storage for hash data
    \Input iHashLen     - length of hash and hash data buffer
    \Input *pData       - data to hash
    \Input iDataLen     - length of data to hash, or -1 if data is a string

    \Version 01/15/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSHashSha2(uint8_t *pHashBuf, int32_t iHashLen, const uint8_t *pData, int32_t iDataLen)
{
    CryptSha2T Sha2;
    // get data length if it is a string
    if (iDataLen < 0)
    {
        iDataLen = (int32_t)strlen((const char *)pData);
    }
    // create hash of request payload
    CryptSha2Init(&Sha2, iHashLen);
    CryptSha2Update(&Sha2, pData, iDataLen);
    CryptSha2Final(&Sha2, pHashBuf, iHashLen);
}

/*F********************************************************************************/
/*!
    \Function _AWSGetDateTime

    \Description
        Get current datetime in ISO8601 basic format.

    \Input *pStrDateTime    - [out] storage for datetime string
    \Input iDateTimeLen     - length of datetime buffer
    \Input *pStrDate        - [out, optional] storage for date string (YYYYMMDD)
    \Input iDateLen         - length of date buffer

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSGetDateTime(char *pStrDateTime, int32_t iDateTimeLen, char *pStrDate, int32_t iDateLen)
{
    struct tm CurTime;
    uint64_t uTime;
    // get current time
    uTime = ds_timeinsecs();
    // get full date in ISO8601 (basic format)
    ds_timetostr(ds_secstotime(&CurTime, uTime), TIMETOSTRING_CONVERSION_ISO_8601_BASIC, FALSE, pStrDateTime, iDateTimeLen);
    // get date string (optional)
    if (pStrDate != NULL)
    {
        ds_strsubzcpy(pStrDate, iDateLen, pStrDateTime, 8);
    }
}

/*F********************************************************************************/
/*!
    \Function _AWSGetKeyInfo

    \Description
        Extract keyid and key from keyid:key string

    \Input *pStrKeyInfo     - pointer to source keyid:key info
    \Input *pStrId          - [out] storage for keyid
    \Input iIdLen           - size of keyid buffer
    \Input *pStrKey         - [out] storage for key
    \Input iKeyLen          - size of key buffer

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSGetKeyInfo(const char *pStrKeyInfo, char *pStrId, int32_t iIdLen, char *pStrKey, int32_t iKeyLen)
{
    const char *pSep;
    *pStrKey = *pStrId = '\0';
    if ((pSep = strchr(pStrKeyInfo, ':')) != NULL)
    {
        ds_strsubzcpy(pStrId, iIdLen, pStrKeyInfo, (int32_t)(pSep-pStrKeyInfo));
        ds_strnzcpy(pStrKey, pSep+1, iKeyLen);
    }
}

/*F********************************************************************************/
/*!
    \Function _AWSGetHostInfo

    \Description
        Get region from host

    \Input *pStrHost        - hostname, used to derive service and region
    \Input *pStrRegion      - [out] AWS region buffer
    \Input iRegionLen       - length of region buffer

    \Version 12/31/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSGetHostInfo(const char *pStrHost, char *pStrRegion, int32_t iRegionLen)
{
    const char *pDot;
    // skip service name in host header
    pDot = strchr(pStrHost, '.');
    pStrHost = pDot+1;
    // get region from host header
    pDot = strchr(pStrHost, '.');
    ds_strsubzcpy(pStrRegion, iRegionLen, pStrHost, (int32_t)(pDot-pStrHost));
}

/*F********************************************************************************/
/*!
    \Function _AWSSignGetHeaders

    \Description
        Given an HTTP header buffer, creates the Canonical and Signed header lists
        required for AWSV4 signing.

    \Input *pHeader         - HTTP header to parse
    \Input *pCanonHdrBuf    - [out] buffer for canonical header list
    \Input iCanonHdrLen     - lengh of canonical header buffer
    \Input *pSignedHdrBuf   - [out] buffer for signed header list
    \Input iSignedHdrLen    - length of signed header buffer
    \Input *pStrHost        - [out] buffer for hostname from host header
    \Input iHostLen         - length of host buffer
    \Input *pStrMethod      - [out] buffer for method from request
    \Input iMethodLen       - length of method buffer
    \Input *pStrUri         - [out] buffer for URI from request
    \Input iUriLen          - length of URI buffer
    \Input *pStrQuery       - [out] buffer for query from request
    \Input iQueryLen        - length of query buffer

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSSignGetHeaders(const char *pHeader, char *pCanonHdrBuf, int32_t iCanonHdrLen, char *pSignedHdrBuf, int32_t iSignedHdrLen, char *pStrHost, int32_t iHostLen, char *pStrMethod, int32_t iMethodLen, char *pStrUri, int32_t iUriLen, char *pStrQuery, int32_t iQueryLen)
{
    char strName[128], strValue[256], strLine[384];
    const char *pHdrTmp = pHeader;

    // get HTTPRequestMethod, CanonicalURI, and CanonicalQuery from first line of request if this is *not* HTTP2
    if (ds_strnicmp(pHeader, ":method:", 8))
    {
        pHdrTmp = ProtoHttpUtilParseRequest(pHeader, pStrMethod, iMethodLen, pStrUri, iUriLen, pStrQuery, iQueryLen, NULL, NULL);
    }

    // get headers, process them, and add to canonical and signed lists in sorted order
    while (ProtoHttpGetNextHeader(NULL, pHdrTmp, strName, sizeof(strName), strValue, sizeof(strValue), &pHdrTmp) == 0)
    {
        // save copy of host header value
        if (!ds_stricmp(strName, "host") || !ds_stricmp(strName, ":authority"))
        {
            ds_strnzcpy(pStrHost, strValue, iHostLen);
        }
        // save copy of method header value (http2)
        if (!ds_stricmp(strName, ":method"))
        {
            ds_strnzcpy(pStrMethod, strValue, iMethodLen);
        }
        // save copy of path header value (http2)
        if (!ds_stricmp(strName, ":path"))
        {
            ds_strnzcpy(pStrUri, strValue, iUriLen);
        }        
        // http2 pseudo-headers don't include values in their canonical representation
        if (strName[0] == ':')
        {
            strValue[0] = '\0';
        }
        // convert name to lower case
        ds_strtolower(strName);
        // trim spaces in value
        ds_strtrim(strValue);
        // format header line for canonical headers list
        ds_snzprintf(strLine, sizeof(strLine), "%s:%s", strName, strValue);
        // insert header line in canonical header list
        ds_strlistinsert(pCanonHdrBuf, iCanonHdrLen, strLine, '\n');
        // insert header name in signed header list
        ds_strlistinsert(pSignedHdrBuf, iSignedHdrLen, strName, ';');
    }
    // eliminate trailing semicolon in signed header list
    pSignedHdrBuf[strlen(pSignedHdrBuf)-1] = '\0';
}

/*F********************************************************************************/
/*!
    \Function _AWSSignCreateCanonicalRequest

    \Description
        Create a Canonical Request for Signature Version 4 as per
        https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4-create-canonical-request.html

    \Input *pSigBuf         - [out] storage for signature
    \Input iSigLen          - length of signature buffer
    \Input *pHeader         - request header
    \Input *pRequest        - request body
    \Input *pStrDateTime    - datetime string
    \Input *pCanonHdrs      - canonical header list
    \Input *pSignedHdrs     - signed header list
    \Input *pMethod         - request method
    \Input *pUri            - request uri
    \Input *pQuery          - request query

    \Version 12/21/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSSignCreateCanonicalRequest(char *pSigBuf, int32_t iSigLen, const char *pHeader, const char *pRequest, const char *pStrDateTime, const char *pCanonHdrs, const char *pSignedHdrs, const char *pMethod, const char *pUri, const char *pQuery)
{
    uint8_t aHashBuf[CRYPTSHA256_HASHSIZE];
    char strSigTemp[1024], strSignTemp[(CRYPTSHA256_HASHSIZE*2)+1];
    int32_t iOffset;

    // create hash of request payload
    _AWSHashSha2(aHashBuf, sizeof(aHashBuf), (const uint8_t *)pRequest, -1);

    // create CanonicalRequest
    iOffset = 0;
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s\n", pMethod);       // HTTPRequestMethod + '\n' +
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s\n", pUri);          // CanonicalURI + '\n' +
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s\n", pQuery);        // CanonicalQueryString + '\n' +
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s\n", pCanonHdrs);    // CanonicalHeaders + '\n' +
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s\n", pSignedHdrs);   // SignedHeaders + '\n' +
    iOffset += ds_snzprintf(strSigTemp+iOffset, sizeof(strSigTemp)-iOffset, "%s", ds_fmtoctstring_lc(strSignTemp, sizeof(strSignTemp), aHashBuf, sizeof(aHashBuf)));   // HexEncode(Hash(RequestPayload))
    NetPrintfVerbose((AWS_DEBUG, 0, "aws: CanonicalRequest:\n%s\n", strSigTemp));

    // hash the CanonicalRequest
    _AWSHashSha2(aHashBuf, sizeof(aHashBuf), (const uint8_t *)strSigTemp, iOffset);

    // create the octet string version of the hash in output buffer
    ds_fmtoctstring_lc(pSigBuf, iSigLen, aHashBuf, sizeof(aHashBuf));   // HexEncode(Hash(CanonicalRequest))
}

/*F********************************************************************************/
/*!
    \Function _AWSSignCreateStringToSign

    \Description
        Create a String to Sign for Signature Version 4 as per
        https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4-create-string-to-sign.html

    \Input *pSignStr        - [out] storage for string to sign
    \Input iSignLen         - length of sign buffer
    \Input *pSignType       - signing type string
    \Input *pDateTimeStr    - datetime in ISO8601 basic format
    \Input *pKeyPath        - keypath (date/region/service) string
    \Input *pSigBuf         - signature created in Task1

    \Output
        int32_t             - length of string to sign

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AWSSignCreateStringToSign(char *pSignStr, int32_t iSignLen, const char *pSignType, const char *pDateTimeStr, const char *pKeyPath, const char *pSigBuf)
{
    int32_t iOffset = 0;
    iOffset += ds_snzprintf(pSignStr+iOffset, iSignLen-iOffset, "%s\n", pSignType); // AWS4-HMAC-SHA256\n
    iOffset += ds_snzprintf(pSignStr+iOffset, iSignLen-iOffset, "%s\n", pDateTimeStr); // YYYYMMDD'T'HHMMSS'Z'
    iOffset += ds_snzprintf(pSignStr+iOffset, iSignLen-iOffset, "%s/aws4_request\n", pKeyPath); // /YYYYMMDD/<region>/<service>/aws4_request\n
    iOffset += ds_snzprintf(pSignStr+iOffset, iSignLen-iOffset, "%s", pSigBuf); // base16-encoded hashed CanonicalRequest
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _AWSSignCreateSignature

    \Description
        Calculate the Signature for AWS Signature Version 4 as per
        https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4-calculate-signature.html

    \Input *pSigBuf         - [out] storage for signature
    \Input iSigLen          - length of signature buffer
    \Input *pSign           - string to sign
    \Input *pSecret         - secret key
    \Input *pStrDate        - date string
    \Input *pRegion         - AWS region
    \Input *pService        - AWS service

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSSignCreateSignature(uint8_t *pSigBuf, int32_t iSigLen, const char *pSign, const char *pSecret, const char *pStrDate, const char *pRegion, const char *pService)
{
    uint8_t aHmac[CRYPTSHA256_HASHSIZE];
    const char *pRequest = "aws4_request";
    char strKey[256];

    // create signing key
    ds_snzprintf(strKey, sizeof(strKey), "%s%s", "AWS4", pSecret);

    // kDate = HMAC("AWS4" + kSecret, Date); note our CryptHmac parameters are reversed from their documentation
    CryptHmacCalc(aHmac, sizeof(aHmac), (const uint8_t *)pStrDate, (int32_t)strlen(pStrDate), (const uint8_t *)strKey, (int32_t)strlen(strKey), CRYPTHASH_SHA256);
    // kRegion = HMAC(kDate, Region)
    CryptHmacCalc(aHmac, sizeof(aHmac), (const uint8_t *)pRegion, (int32_t)strlen(pRegion), aHmac, sizeof(aHmac), CRYPTHASH_SHA256);
    // kService = HMAC(kRegion, Service)
    CryptHmacCalc(aHmac, sizeof(aHmac), (const uint8_t *)pService, (int32_t)strlen(pService), aHmac, sizeof(aHmac), CRYPTHASH_SHA256);
    // kSigning = HMAC(kService, "aws4_request")
    CryptHmacCalc(aHmac, sizeof(aHmac), (const uint8_t *)pRequest, (int32_t)strlen(pRequest), aHmac, sizeof(aHmac), CRYPTHASH_SHA256);

    // sign the string
    CryptHmacCalc(aHmac, sizeof(aHmac), (const uint8_t *)pSign, (int32_t)strlen(pSign), aHmac, sizeof(aHmac), CRYPTHASH_SHA256);
    // copy signature to output
    ds_memcpy_s(pSigBuf, iSigLen, aHmac, sizeof(aHmac));
}

/*F********************************************************************************/
/*!
    \Function _AWSSignCreateAuthorization

    \Description
        Create the Authorization header for adding to the HTTP request
        https://docs.aws.amazon.com/en_us/general/latest/gr/sigv4-add-signature-to-request.html

    \Input *pAuthBuf        - [out] storage for authorization header
    \Input iAuthLen         - length of auth buffer
    \Input *pStrSig         - signature
    \Input *pKeyId          - key id 
    \Input *pStrDateTime    - datetime string
    \Input *pKeyPath        - keypath (date/region/service) string
    \Input *pSignedHdrs     - signed headers

    \Output
        int32_t             - length of authorization header

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AWSSignCreateAuthorization(char *pAuthBuf, int32_t iAuthLen, const char *pStrSig, const char *pKeyId, const char *pStrDateTime, const char *pKeyPath, const char *pSignedHdrs)
{
    int32_t iOffset;
    // Authorization: algorithm Credential=access key ID/credential scope, SignedHeaders=SignedHeaders, Signature=signature
    iOffset = 0;
    iOffset += ds_snzprintf(pAuthBuf+iOffset, iAuthLen-iOffset, "Authorization: AWS4-HMAC-SHA256 ");
    iOffset += ds_snzprintf(pAuthBuf+iOffset, iAuthLen-iOffset, "Credential=%s/%s/aws4_request, ", pKeyId, pKeyPath);
    iOffset += ds_snzprintf(pAuthBuf+iOffset, iAuthLen-iOffset, "SignedHeaders=%s, ", pSignedHdrs);
    iOffset += ds_snzprintf(pAuthBuf+iOffset, iAuthLen-iOffset, "Signature=%s\r\n", pStrSig);
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _AWSWriteEventPrelude

    \Description
        Write binary event prelude to buffer

    \Input *pBuffer     - [out] buffer to write prelude to
    \Input iHdrLen      - length of header
    \Input iMsgLen      - length of message

    \Version 01/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSWriteEventPrelude(uint8_t *pBuffer, int32_t iHdrLen, int32_t iMsgLen)
{
    uint32_t uCrc32;
    // write total length (including message_crc)
    pBuffer[0] = (uint8_t)(iMsgLen>>24);
    pBuffer[1] = (uint8_t)(iMsgLen>>16);
    pBuffer[2] = (uint8_t)(iMsgLen>>8);
    pBuffer[3] = (uint8_t)(iMsgLen>>0);
    // write headers length
    pBuffer[4] = (uint8_t)(iHdrLen>>24);
    pBuffer[5] = (uint8_t)(iHdrLen>>16);
    pBuffer[6] = (uint8_t)(iHdrLen>>8);
    pBuffer[7] = (uint8_t)(iHdrLen>>0);
    // calculate and write prelude_crc
    uCrc32 = NetCrc32(pBuffer, 8, NULL);
    pBuffer[8] = (uint8_t)(uCrc32>>24);
    pBuffer[9] = (uint8_t)(uCrc32>>16);
    pBuffer[10] = (uint8_t)(uCrc32>>8);
    pBuffer[11] = (uint8_t)(uCrc32>>0);
}

/*F********************************************************************************/
/*!
    \Function _AWSWriteEventHeader

    \Description
        Write binary event header to buffer

    \Input *pBuffer     - [out] buffer to write header to
    \Input iBufSize     - buffer size
    \Input *pHeader     - name of header to write
    \Input *pValue      - header value data
    \Input iValueSize   - size of header value
    \Input eHeaderValue - header value type

    \Output
        int32_t         - size of header written

    \Version 01/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AWSWriteEventHeader(uint8_t *pBuffer, int32_t iBufSize, const char *pHeader, const uint8_t *pValue, int32_t iValueSize, AWSHeaderValueE eHeaderValue)
{
    int32_t iOffset=0, iStrLen = (int32_t)strlen(pHeader);
    // write header length
    pBuffer[iOffset++] = (uint8_t)iStrLen;
    // copy header name
    ds_memcpy_s(pBuffer+iOffset, iBufSize-iOffset, pHeader, iStrLen);
    iOffset += iStrLen;
    // copy header value type
    pBuffer[iOffset++] = (uint8_t)eHeaderValue;
    // copy header value length
    if ((eHeaderValue == AWS_BYTE_ARRAY) || (eHeaderValue == AWS_STRING))
    {
        pBuffer[iOffset++] = (uint8_t)(iValueSize>>8);
        pBuffer[iOffset++] = (uint8_t)(iValueSize>>0);
    }
    // copy header value
    #if EA_SYSTEM_LITTLE_ENDIAN
    if ((eHeaderValue == AWS_SHORT) || (eHeaderValue == AWS_INTEGER) || (eHeaderValue == AWS_LONG) || (eHeaderValue == AWS_TIMESTAMP))
    {
        int32_t iByte, iData;
        for (iByte = 0, iData = iValueSize-1; iByte < iValueSize; iByte += 1)
        {
            pBuffer[iOffset++] = pValue[iData--];
        }
    }
    else
    #endif
    {
        ds_memcpy_s(pBuffer+iOffset, iBufSize-iOffset, pValue, iValueSize);
        iOffset += iValueSize;
    }
    // return offset to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _AWSWriteSignEvent

    \Description
        Sign an AWS binary event

    \Input *pSignature      - [out] buffer to write signature to
    \Input iSigSize         - size of signature to write
    \Input *pHead           - header of event to sign
    \Input iHeadSize        - size of header
    \Input *pData           - data to sign
    \Input iDataSize        - size of data (may be zero)
    \Input *pSignInfo       - [in/out] signing info for event

    \Notes
        The signature field in the signing info is used to sign the event, and
        then updated with the new signature, which AWS calls "signature chaining".

        String stringToSign =
            "AWS4-HMAC-SHA256-PAYLOAD" + "\n"
            DATE + "\n" +
            KEYPATH + "\n" +
            Hex(priorSignature) + "\n" +
            HexHash(nonSigHeaders) + "\n" +
            HexHash(payload)

    \Version 01/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static void _AWSWriteSignEvent(uint8_t *pSignature, int32_t iSigSize, const uint8_t *pHead, int32_t iHeadSize, const uint8_t *pData, int32_t iDataSize, AWSSignInfoT *pSignInfo)
{
    char strSign[1024], strDateTime[32], strDate[9], strSignTemp[(CRYPTSHA256_HASHSIZE*2)+1];
    uint8_t aHashBuf[CRYPTSHA256_HASHSIZE];
    int32_t iOffset;
    // get datetime
    _AWSGetDateTime(strDateTime, sizeof(strDateTime), strDate, sizeof(strDate));
    // create the string to signl includes header, datetime, keypath, and priorSignature
    iOffset = _AWSSignCreateStringToSign(strSign, sizeof(strSign), "AWS4-HMAC-SHA256-PAYLOAD", strDateTime, pSignInfo->strKeyPath, pSignInfo->strSignature);
    // append hexhash of non-signed headers
    _AWSHashSha2(aHashBuf, sizeof(aHashBuf), pHead, iHeadSize);
    iOffset += ds_snzprintf(strSign+iOffset, sizeof(strSign)-iOffset, "\n%s\n", ds_fmtoctstring_lc(strSignTemp, sizeof(strSignTemp), aHashBuf, sizeof(aHashBuf)));   // HexEncode(Hash(nonSigHeaders)) + \n
    // append hexhash of payload
    _AWSHashSha2(aHashBuf, sizeof(aHashBuf), pData, iDataSize);
    iOffset += ds_snzprintf(strSign+iOffset, sizeof(strSign)-iOffset, "%s", ds_fmtoctstring_lc(strSignTemp, sizeof(strSignTemp), aHashBuf, sizeof(aHashBuf)));   // HexEncode(Hash(payload))
    // debug logging of string to sign
    NetPrintfVerbose((AWS_DEBUG, 0, "aws: EventStringToSign:\n%s\n", strSign));
    // sign the string; String signature = HMAC(derived_signing_key, stringToSign);
    _AWSSignCreateSignature(pSignature, iSigSize, strSign, pSignInfo->strKey, strDate, pSignInfo->strRegion, pSignInfo->strService);
    // update signing key (signature chaining)
    ds_strnzcpy(pSignInfo->strSignature, ds_fmtoctstring_lc(strSignTemp, sizeof(strSignTemp), pSignature, iSigSize), sizeof(pSignInfo->strSignature));
}

/*F********************************************************************************/
/*!
    \Function _AWSReadEventHeader

    \Description
        Read an event header from an event

    \Input *pBuffer         - buffer pointing to event to read
    \Input iBufSize         - size of buffer
    \Input *pHeaderName     - [out] buffer to store header name
    \Input iNameSize        - size of header name buffer
    \Input *pHeaderValue    - [out] buffer to store header value
    \Input *pValueSize      - [in] size of value buffer, [out] size of value
    \Input *pValueType      - [out] storage for value type

    \Output
        int32_t             - size of event

    \Version 01/17/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _AWSReadEventHeader(const uint8_t *pBuffer, int32_t iBufSize, char *pHeaderName, int32_t iNameSize, uint8_t *pHeaderValue, int32_t *pValueSize, AWSHeaderValueE *pValueType)
{
    int32_t iHdrLen, iValLen=0;
    int32_t iOffset=0;
    // get length
    iHdrLen = pBuffer[iOffset++];
    // copy string
    ds_strsubzcpy(pHeaderName, iNameSize, (const char *)pBuffer+iOffset, iHdrLen);
    iOffset += iHdrLen;
    // get value type
    *pValueType = (AWSHeaderValueE)pBuffer[iOffset++];
    // get value length (assume string for now) $$todo - add support for other types
    if ((*pValueType == AWS_BYTE_ARRAY) || (*pValueType == AWS_STRING))
    {
        iValLen = pBuffer[iOffset++] << 8;
        iValLen |= pBuffer[iOffset++];
    }
    // copy value
    ds_memcpy_s(pHeaderValue, *pValueSize, pBuffer+iOffset, iValLen);
    iOffset += iValLen;
    // null-terminate if string
    if (*pValueType == AWS_STRING)
    {
        pHeaderValue[iValLen] = '\0';
    }
    *pValueSize = iValLen;
    return(iOffset);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function AWSSignSigV4

    \Description
        Sign request as per Amazon Signature Version 4 Signing Process:
        https://docs.aws.amazon.com/en_us/general/latest/gr/signature-version-4.html

    \Input *pHeader         - http header
    \Input iHeaderSize      - size of header buffer
    \Input *pRequest        - request body
    \Input *pKeyInfo        - signing key info ("keyid:key")
    \Input *pService        - AWS service
    \Input *pSignInfo       - [out, optional] storage for signing info for later use

    \Output
        int32_t             - updated header length

    \Version 12/26/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t AWSSignSigV4(char *pHeader, int32_t iHeaderSize, const char *pRequest, const char *pKeyInfo, const char *pService, AWSSignInfoT *pSignInfo)
{
    char strCanonHdrs[512]="", strSignedHdrs[256]="", strDateTime[32]="", strHost[256], strQuery[128]="";
    char strSig[(CRYPTSHA256_HASHSIZE*2)+1];
    char strRegion[32], strSign[256], strDate[9], strMethod[8], strUri[128];
    char strKey[64], strKeyId[32], strKeyPath[128];
    uint8_t aSigBuf[CRYPTSHA256_HASHSIZE];
    int32_t iOffset;

    // get/assemble info we need to sign request

    // get DateTime for X-Amz-Date header
    _AWSGetDateTime(strDateTime, sizeof(strDateTime), strDate, sizeof(strDate));
    // append DateTime to header so it will be included in canonical/signed headers
    iOffset = (int32_t)strlen(pHeader);
    iOffset += ds_snzprintf(pHeader+iOffset, iHeaderSize-iOffset, "X-Amz-Date: %s\r\n", strDateTime);
    // create canonical and signed header lists
    _AWSSignGetHeaders(pHeader, strCanonHdrs, sizeof(strCanonHdrs), strSignedHdrs, sizeof(strSignedHdrs), strHost, sizeof(strHost), strMethod, sizeof(strMethod), strUri, sizeof(strUri), strQuery, sizeof(strQuery));
    // extract region and date info
    _AWSGetHostInfo(strHost, strRegion, sizeof(strRegion));
    // extract keyid and key from secret
    _AWSGetKeyInfo(pKeyInfo, strKeyId, sizeof(strKeyId), strKey, sizeof(strKey));
    // create keypath
    ds_snzprintf(strKeyPath, sizeof(strKeyPath), "%s/%s/%s", strDate, strRegion, pService);

    // sign the request as per https://docs.aws.amazon.com/en_us/general/latest/gr/signature-version-4.html

    // Task 1: Create a Canonical Request for Signature Version 4
    _AWSSignCreateCanonicalRequest(strSig, sizeof(strSig), pHeader, pRequest, strDateTime, strCanonHdrs, strSignedHdrs, strMethod, strUri, strQuery);
    // Task 2: Create a String to Sign for Signature Version 4
    _AWSSignCreateStringToSign(strSign, sizeof(strSign), "AWS4-HMAC-SHA256", strDateTime, strKeyPath, strSig);
    NetPrintfVerbose((AWS_DEBUG, 0, "aws: StringToSign:\n%s\n", strSign));
    // Task 3: Calculate the Signature for AWS Signature Version 4 and convert it to a hexstring
    _AWSSignCreateSignature(aSigBuf, sizeof(aSigBuf), strSign, strKey, strDate, strRegion, pService);
    ds_fmtoctstring_lc(strSig, sizeof(strSig), aSigBuf, sizeof(aSigBuf));
    // Task 4: Create the Authorization header
    iOffset += _AWSSignCreateAuthorization(pHeader+iOffset, iHeaderSize-iOffset, strSig, strKeyId, strDateTime, strKeyPath, strSignedHdrs);

    // save signing info
    if (pSignInfo != NULL)
    {
        ds_strnzcpy(pSignInfo->strService, pService, sizeof(pSignInfo->strService));
        ds_strnzcpy(pSignInfo->strRegion, strRegion, sizeof(pSignInfo->strRegion));
        ds_strnzcpy(pSignInfo->strKeyPath, strKeyPath, sizeof(pSignInfo->strKeyPath));
        ds_strnzcpy(pSignInfo->strSignature, strSig, sizeof(pSignInfo->strSignature));
        ds_strnzcpy(pSignInfo->strKey, strKey, sizeof(pSignInfo->strKey));
    }

    // return updated header length
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function AWSWriteEvent

    \Description
        Write an AWS signed binary event

    \Input *pBuffer         - [out] buffer to write event
    \Input iBufSize         - size of buffer
    \Input *pData           - event data
    \Input *pDataSize       - [in/out] size of input data available/written
    \Input *pEventType      - event type name
    \Input *pSignInfo       - [in/out] signing info for event

    \Output
        int32_t             - size of event data written

    \Notes
        The signature field in the signing info is used to sign the event, and
        then updated with the new signature; AWS calls this "signature chaining".

    \Version 01/16/2019 (jbrookes)
*/
/********************************************************************************F*/
int32_t AWSWriteEvent(uint8_t *pBuffer, int32_t iBufSize, const uint8_t *pData, int32_t *pDataSize, const char *pEventType, AWSSignInfoT *pSignInfo)
{
    int32_t iOffset=0, iHdrOff, iHdrLen, iAudOff, iAudLen, iSigOff, uCrc32;
    uint64_t uTimeMs = ds_timeinsecs() * 1000;
    static const uint8_t aZeroBuf[CRYPTSHA256_HASHSIZE] = { 0 };
    static const char *pContentType = "application/octet-stream";
    static const char *pMessageType = "event";
    const uint8_t *pSignData=(const uint8_t *)"";
    int32_t iSignDataSize=0;

    // reserve room for outer chunk prelude
    iOffset += AWS_EVENT_PRELUDE_SIZE;

    // write outer chunk headers
    iOffset += _AWSWriteEventHeader(pBuffer+iOffset, iBufSize-iOffset, ":date", (const uint8_t *)&uTimeMs, sizeof(uTimeMs), AWS_TIMESTAMP);
    iHdrOff = iOffset;
    iOffset += _AWSWriteEventHeader(pBuffer+iOffset, iBufSize-iOffset, ":chunk-signature", aZeroBuf, sizeof(aZeroBuf), AWS_BYTE_ARRAY);
    iSigOff = iOffset-sizeof(aZeroBuf);
    iHdrLen = iOffset-AWS_EVENT_PRELUDE_SIZE;

    if (pData != NULL)
    {
        // reserve room for event chunk prelude
        iAudOff = iOffset;
        iOffset += AWS_EVENT_PRELUDE_SIZE;

        // write event chunk headers
        iOffset += _AWSWriteEventHeader(pBuffer+iOffset, iBufSize-iOffset, ":event-type", (const uint8_t *)pEventType, (int32_t)strlen(pEventType), AWS_STRING);
        iOffset += _AWSWriteEventHeader(pBuffer+iOffset, iBufSize-iOffset, ":content-type", (const uint8_t *)pContentType, (int32_t)strlen(pContentType), AWS_STRING);
        iOffset += _AWSWriteEventHeader(pBuffer+iOffset, iBufSize-iOffset, ":message-type", (const uint8_t *)pMessageType, (int32_t)strlen(pMessageType), AWS_STRING);
        iAudLen = iOffset-iAudOff-AWS_EVENT_PRELUDE_SIZE;

        // calculate how much data we can write, including the event chunk message_crc and outer chunk message_crc
        *pDataSize = DS_MIN(*pDataSize, iBufSize-iOffset-4-4);
        // copy event data
        ds_memcpy(pBuffer+iOffset, pData, *pDataSize);
        iOffset += *pDataSize;

        // finish event chunk (add four bytes for crc32)
        _AWSWriteEventPrelude(pBuffer+iAudOff, iAudLen, iOffset+4-iAudOff);

        // calculate and write event chunk message_crc
        uCrc32 = NetCrc32(pBuffer+iAudOff, iOffset-iAudOff, NULL);
        pBuffer[iOffset++] = (uint8_t)(uCrc32>>24);
        pBuffer[iOffset++] = (uint8_t)(uCrc32>>16);
        pBuffer[iOffset++] = (uint8_t)(uCrc32>>8);
        pBuffer[iOffset++] = (uint8_t)(uCrc32>>0);

        // locate data to sign
        pSignData = pBuffer+iAudOff;
        iSignDataSize = iOffset-iAudOff;
    }

    // sign the event
    _AWSWriteSignEvent(pBuffer+iSigOff, CRYPTSHA256_HASHSIZE, pBuffer+AWS_EVENT_PRELUDE_SIZE, iHdrOff-AWS_EVENT_PRELUDE_SIZE, pSignData, iSignDataSize, pSignInfo);

    // finish outer chunk (add four bytes for crc32)
    _AWSWriteEventPrelude(pBuffer, iHdrLen, iOffset+4);

    // calculate and write outer chunk message_crc
    uCrc32 = NetCrc32(pBuffer, iOffset, NULL);
    pBuffer[iOffset++] = (uint8_t)(uCrc32>>24);
    pBuffer[iOffset++] = (uint8_t)(uCrc32>>16);
    pBuffer[iOffset++] = (uint8_t)(uCrc32>>8);
    pBuffer[iOffset++] = (uint8_t)(uCrc32>>0);

    // return offset to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function AWSReadEvent

    \Description
        Read an AWS signed binary event

    \Input *pBuffer         - buffer pointing to event to read
    \Input iBufSize         - size of buffer
    \Input *pEventType      - [out] buffer to store header event type
    \Input iEventSize       - size of event type buffer
    \Input *pMessage        - [out] buffer to store event message
    \Input *pMessageSize    - [in] size of message buffer, [out] size of message

    \Output
        int32_t             - size of event

    \Notes
        This function does not attempt to verify the CRCs.

    \Version 01/17/2019 (jbrookes)
*/
/********************************************************************************F*/
int32_t AWSReadEvent(const uint8_t *pBuffer, int32_t iBufSize, char *pEventType, int32_t iEventSize, char *pMessage, int32_t *pMessageSize)
{
    char strHeaderName[128], strHeaderValue[1024];
    int32_t iOffset=0, iValueLen;
    int32_t iMsgLen, iHdrLen;
    AWSHeaderValueE eValueType;
    int32_t iResult = -1;

    // make sure we have enough data for prelude
    if (iBufSize < AWS_EVENT_PRELUDE_SIZE)
    {
        return(iResult);
    }

    // read overall message length
    iMsgLen  = pBuffer[iOffset++] << 24;
    iMsgLen |= pBuffer[iOffset++] << 16;
    iMsgLen |= pBuffer[iOffset++] << 8;
    iMsgLen |= pBuffer[iOffset++] << 0;
    // read header length
    iHdrLen  = pBuffer[iOffset++] << 24;
    iHdrLen |= pBuffer[iOffset++] << 16;
    iHdrLen |= pBuffer[iOffset++] << 8;
    iHdrLen |= pBuffer[iOffset++] << 0;
    // skip crc $$todo - verify this?
    iOffset += 4;

    // calculate actual message length (total-headers-prelude-crc32
    iMsgLen = iMsgLen-iHdrLen-AWS_EVENT_PRELUDE_SIZE-4;

    // get message headers
    while (iOffset < (iHdrLen+AWS_EVENT_PRELUDE_SIZE))
    {
        iOffset += _AWSReadEventHeader(pBuffer+iOffset, iBufSize-iOffset, strHeaderName, sizeof(strHeaderName), (uint8_t *)strHeaderValue, (iValueLen=(int32_t)sizeof(strHeaderValue), &iValueLen), &eValueType);
        if (eValueType != AWS_STRING)
        {
            NetPrintfVerbose((AWS_DEBUG, 0, "aws: %s: [%d bytes] type=%d\n", strHeaderName, iValueLen, eValueType));
            continue;
        }

        NetPrintfVerbose((AWS_DEBUG, 0, "aws: %s: %s\n", strHeaderName, strHeaderValue));
        if (!ds_stricmp(strHeaderName, ":event-type") || !ds_stricmp(strHeaderName, ":exception-type"))
        {
            ds_strnzcpy(pEventType, strHeaderValue, iEventSize);
        }
    }

    // copy message body
    ds_strsubzcpy(pMessage, *pMessageSize, (const char *)pBuffer+iOffset, iMsgLen);
    *pMessageSize = iMsgLen;
    // skip past message
    iOffset += iMsgLen;
    // skip past crc32
    iOffset += 4;
    // return offset past event to caller
    return(iOffset);
}
