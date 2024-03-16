/*H*************************************************************************************/
/*!
    \File sslstress.c

    \Description
        A smoke tester for TLS/SSL, designed to connect to a series of servers while
        exercising options (version, cipher) to validate basic connectability across
        the protocol feature set supported by ProtoSSL.

    \Copyright
        Copyright (c) Electronic Arts 2017.

    \Version 08/03/2017 (jbrookes) First Version
*/
/*************************************************************************************H*/

/*** Example Usage *********************************************************************/

/*
  Test TLS1.3 servers:
    -cert e:\temp\testcerts\DSTRootCAX3.crt -minvers 4 -minciph 14 https://enabled.tls13.com https://tls13.crypto.mozilla.org/ https://tls.ctf.network/ https://rustls.jbp.io/ https://h2o.examp1e.net https://www.mew.org/
  Test TLS1.2 servers:
    https://www.google.com https://www.yahoo.com https://www.amazon.com https://facebook.com https://badssl.com
*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dirtysock includes
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protossl.h"

// zlib includes
#include "libsample/zlib.h"
#include "libsample/zmem.h"
#include "libsample/zfile.h"

/*** Defines ***************************************************************************/

#define SSLTEST_MAXCIPHER_PRE10      1      // index of max cipher supported prior to tls1.0
#define SSLTEST_MAXCIPHER_PRE12      5      // index of max cipher supported prior to tls1.2
#define SSLTEST_MAXCIPHER_PRE13     19      // index of max cipher supported prior to tls1.3
#define SSLTEST_CIPHER_ALL          23      // index of entry supporting all ciphers

enum
{
    SSLTEST_STAGE_SETUP = 0,
    SSLTEST_STAGE_RUNNING,
    SSLTEST_STAGE_RESULT,
    SSLTEST_STAGE_COMPLETE
};

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct SsltestOptionsT
{
    int32_t iMinTest;
    int32_t iMaxTest;
    int32_t iMinServ;
    int32_t iMaxServ;
    int32_t iMinVers;
    int32_t iMaxVers;
    int32_t iMinCiph;
    int32_t iMaxCiph;
    uint8_t bNcrt;
    uint8_t bSkip;
    uint8_t _pad[2];
} SsltestOptionsT;


typedef struct SsltestStateT
{
    int32_t iTest;
    int32_t iServ;
    int32_t iVers;
    int32_t iCiph;
    uint8_t *pClientCert;
    int32_t iCertSize;
    uint8_t *pClientKey;
    int32_t iKeySize;
    char strResult[128];
} SsltestStateT;

// test function type
typedef void(SsltestTestFuncT)(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage);

// test type
typedef struct SsltestTestT
{
    SsltestTestFuncT *pTestFunc;
    char *pTestName;
} SsltestTestT;

/*** Function Prototypes ***************************************************************/

static void test_cipher_and_version(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage);
static void test_resume(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage);
static void test_basic(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage);

/*** Variables *************************************************************************/

// Private variables

static uint16_t _ssltest_versions[] =
{
    PROTOSSL_VERSION_TLS1_0,
    PROTOSSL_VERSION_TLS1_1,
    PROTOSSL_VERSION_TLS1_2,
    PROTOSSL_VERSION_TLS1_3
};

static const char *_ssltest_tlsnames[] =
{
    "TLS1.0",
    "TLS1.1",
    "TLS1.2",
    "TLS1.3",
};

// protossl ciphers - matches list order in protossl.h
static uint32_t _ssltest_ciphers[] =
{
    // SSLv3 cipher suites
    PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA,
    PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA,
    // TLS1.0 cipher suites
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
    // the following cipher suites are TLS1.2+ only
    PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256,
    PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256,
    PROTOSSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256,
    PROTOSSL_CIPHER_RSA_WITH_AES_256_GCM_SHA384,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
    PROTOSSL_CIPHER_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
    PROTOSSL_CIPHER_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
    // the following cipher suites are TLS1.3+ only
    PROTOSSL_CIPHER_AES_128_GCM_SHA256,
    PROTOSSL_CIPHER_AES_256_GCM_SHA384,
    PROTOSSL_CIPHER_CHACHA20_POLY1305_SHA256,
    // all ciphers
    PROTOSSL_CIPHER_ALL|PROTOSSL_CIPHER_ALL_13
};

static const char *_ssltest_ciphernames[] =
{
    // SSLv3 cipher suites
    "TLS_RSA_WITH_AES_128_CBC_SHA",
    "TLS_RSA_WITH_AES_256_CBC_SHA",
    // TLS1.0 cipher suites
    "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA",
    "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA",
    "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA",
    "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA",
    // TLS1.2 cipher suites
    "TLS_RSA_WITH_AES_128_CBC_SHA256",
    "TLS_RSA_WITH_AES_256_CBC_SHA256",
    "TLS_RSA_WITH_AES_128_GCM_SHA256",
    "TLS_RSA_WITH_AES_256_GCM_SHA384",
    "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256",
    "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384",
    "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
    "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
    "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256",
    "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256",
    "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384",
    "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",
    "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384",
    "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256",
    // TLS1.3 cipher suites
    "TLS_AES_128_GCM_SHA256",
    "TLS_AES_256_GCM_SHA384",
    "TLS_CHACHA20_POLY1305_SHA256",
    // a synthetic for all ciphers
    "TLS_ALL_CIPHERS"
};

static SsltestTestT _ssltest_tests[] =
{
    { test_resume,             "resume" },
    { test_cipher_and_version, "cipher and version" },
    { test_basic,              "basic" }
};

// Public variables


/*** Private Functions ******************************************************************/

static int32_t ssltest_zprintf_hook(void *pParm, const char *pText)
{
    // on PC we want output to console in addition to debug output
    #if defined(DIRTYCODE_PC)
    printf("%s", pText);
    #endif
    // don't suppress debug output
    return(1);
}

static int32_t network_startup(void)
{
    int32_t iResult=0, iStatus, iTimeout;

    // start network
    NetConnStartup("-servicename=sslstress");

    // bring up the interface
    NetConnConnect(NULL, NULL, 0);

    // wait for network interface activation
    for (iTimeout = NetTick() + 15*1000; ; )
    {
        // update network
        NetConnIdle();

        // get current status
        iStatus = NetConnStatus('conn', 0, NULL, 0);
        if ((iStatus == '+onl') || ((iStatus >> 24) == '-'))
        {
            break;
        }

        // check for timeout
        if (iTimeout < (signed)NetTick())
        {
            ZPrintf("ssltest: timeout waiting for interface activation\n");
            break;
        }

        // give time to other threads
        NetConnSleep(500);
    }

    // check result code
    if ((iStatus = NetConnStatus('conn', 0, NULL, 0)) == '+onl')
    {
        ZPrintf("ssltest: interface active\n");
        iResult = 1;
    }
    else if ((iStatus >> 24) == '-')
    {
        ZPrintf("ssltest: error %C bringing up interface\n", iStatus);
    }

    // return result to caller
    return(iResult);
}

static uint8_t *load_pem(const char *pFilename, int32_t *pCertSize)
{
    char *pCertBuf;
    int32_t iFileSize;

    // load certificate file
    if ((pCertBuf = (char *)ZFileLoad(pFilename, &iFileSize, ZFILE_OPENFLAG_RDONLY)) == NULL)
    {
        return(NULL);
    }

    // calculate length
    *pCertSize = iFileSize;
    // return to caller
    return((uint8_t *)pCertBuf);
}

static void load_ca(const char *pCertPath)
{
    const uint8_t *pFileData;
    int32_t iFileSize, iResult = 0;

    // try and open file
    if ((pFileData = (const uint8_t *)ZFileLoad(pCertPath, &iFileSize, ZFILE_OPENFLAG_RDONLY | ZFILE_OPENFLAG_BINARY)) != NULL)
    {
        iResult = ProtoHttpSetCACert(pFileData, iFileSize);
        ZMemFree((void *)pFileData);
    }
    ZPrintf("ssltest: load of certificate '%s' %s\n", pCertPath, (iResult > 0) ? "succeeded" : "failed");
}

static void load_cert(SsltestStateT *pState, const char *pCertPath)
{
    // load the cert
    pState->pClientCert = load_pem(pCertPath, &pState->iCertSize);
    // output result
    ZPrintf("ssltest: load of client certificate '%s' %s\n", pCertPath, (pState->pClientCert != NULL) ? "succeeded" : "failed");
}

static void load_key(SsltestStateT *pState, const char *pCertPath)
{
    // load the key
    pState->pClientKey = load_pem(pCertPath, &pState->iKeySize);
    // output result
    ZPrintf("ssltest: load of client key '%s' %s\n", pCertPath, (pState->pClientKey != NULL) ? "succeeded" : "failed");
}

// test all ciphers and tls versions between min and max
static void test_cipher_and_version(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage)
{
    if (iStage == SSLTEST_STAGE_SETUP)
    {
        // disable session resumption to force a new connection every attempt
        ProtoHttpControl(pHttpRef, 'resu', 0, 0, NULL);
    }
    else if (iStage == SSLTEST_STAGE_RUNNING)
    {
        // select next cipher/version
        if (++pState->iCiph >= pOptions->iMaxCiph)
        {
            pState->iVers += 1;
            pState->iCiph = pOptions->iMinCiph;
        }
        // select next server
        if (pState->iVers >= pOptions->iMaxVers)
        {
            pState->iServ += 1;
            pState->iVers = pOptions->iMinVers;
        }
    }

    if (iStage < SSLTEST_STAGE_RESULT)
    {
        // select specific cipher
        ProtoHttpControl(pHttpRef, 'ciph', _ssltest_ciphers[pState->iCiph], 0, NULL);
    }

    // format result text
    if (iStage == SSLTEST_STAGE_RESULT)
    {
        ds_snzprintf(pState->strResult, sizeof(pState->strResult), "vers=%x", ProtoHttpStatus(pHttpRef, 'vers', NULL, 0));
    }
}

// test session resumption in all tls versions
static void test_resume(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage)
{
    static int32_t _iTest;

    if (iStage == SSLTEST_STAGE_SETUP)
    {
        // enable session resumption
        ProtoHttpControl(pHttpRef, 'resu', 1, 0, NULL);

        // enable all ciphers
        pState->iCiph = SSLTEST_CIPHER_ALL;
        ProtoHttpControl(pHttpRef, 'ciph', _ssltest_ciphers[pState->iCiph], 0, NULL);

        // since we want to do tests twice, track that here
        _iTest = -1;
    }

    // since we're testing resume, we want to connect twice (first non-resuming, second resuming)
    if (iStage < SSLTEST_STAGE_RESULT)
    {
        if (++_iTest == 2)
        {
            // select next version
            pState->iVers += 1;
            // if past max, select next server
            if (pState->iVers >= pOptions->iMaxVers)
            {
                pState->iServ += 1;
                pState->iVers = pOptions->iMinVers;
            }
            _iTest = 0;
        }
    }

    // format result text
    if (iStage == SSLTEST_STAGE_RESULT)
    {
        char strCipher[64] = "";
        ProtoHttpStatus(pHttpRef, 'ciph', strCipher, sizeof(strCipher));
        ds_snzprintf(pState->strResult, sizeof(pState->strResult), "vers=%x, ciph=%s, resume=%s", ProtoHttpStatus(pHttpRef, 'vers', NULL, 0), strCipher,
            ProtoHttpStatus(pHttpRef, 'resu', NULL, 0) ? "true" : "false");
    }
}

// test a single basic connection with all ciphers enabled
static void test_basic(ProtoHttpRefT *pHttpRef, SsltestOptionsT *pOptions, SsltestStateT *pState, int32_t iStage)
{
    if (iStage == SSLTEST_STAGE_SETUP)
    {
        // enable all ciphers
        pState->iCiph = SSLTEST_CIPHER_ALL;
        ProtoHttpControl(pHttpRef, 'ciph', _ssltest_ciphers[pState->iCiph], 0, NULL);
        //ProtoHttpControl(pHttpRef, 'crvd', 2, 0, NULL);
    }

    if (iStage == SSLTEST_STAGE_RUNNING)
    {
        // select next server
        if (pState->iVers >= pOptions->iMaxVers)
        {
            pState->iServ += 1;
            pState->iVers = pOptions->iMinVers;
        }
    }

    // handle results formatting
    if (iStage == SSLTEST_STAGE_RESULT)
    {
        // format result text
        char strCipher[64] = "", strSigAlg[32] = "salg=";
        int32_t iSigAlg;
        ProtoHttpStatus(pHttpRef, 'ciph', strCipher, sizeof(strCipher));
        iSigAlg = ProtoHttpStatus(pHttpRef, 'salg', strSigAlg+5, sizeof(strSigAlg)-5);
        ds_snzprintf(pState->strResult, sizeof(pState->strResult), "vers=%x, ciph=%s %s", ProtoHttpStatus(pHttpRef, 'vers', NULL, 0), strCipher, iSigAlg ? strSigAlg : "");
    }

    // handle completion
    if (iStage == SSLTEST_STAGE_COMPLETE)
    {
        // select next version
        pState->iVers += 1;
    }
}

static const char **process_args(SsltestStateT *pState, SsltestOptionsT *pOptions, int32_t iArgc, const char *pArgv[])
{
    static const char *strServerDefault[] = { "https://www.google.com" }, **pServers;
    int32_t iArg;

    // echo options
    for (iArg = 0; iArg < iArgc; iArg += 1)
    {
        ZPrintf("%s ", pArgv[iArg]);
    }
    ZPrintf("\n");

    ds_memclr(pOptions, sizeof(*pOptions));

    // init default options
    pOptions->iMinVers = 0;
    pOptions->iMaxVers = (int32_t)sizeof(_ssltest_versions)/sizeof(_ssltest_versions[0]);
    pOptions->iMinCiph = 0;
    pOptions->iMaxCiph = (int32_t)sizeof(_ssltest_ciphers)/sizeof(_ssltest_ciphers[0]) - 1;  // remove SSLTEST_CIPHER_ALL
    pOptions->iMinTest = 0;
    pOptions->iMaxTest = (int32_t)sizeof(_ssltest_tests)/sizeof(_ssltest_tests[0]);
    pOptions->bSkip = FALSE;

    // pick off command-line options
    for (iArg = 1; (iArg < iArgc) && (pArgv[iArg][0] == '-'); iArg += 1)
    {
        if (!strcmp(pArgv[iArg], "-cert") && ((iArg+1) < iArgc))
        {
            load_ca(pArgv[iArg+1]);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-scrt") && ((iArg+1) < iArgc))
        {
            load_cert(pState, pArgv[iArg+1]);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-skey") && ((iArg+1) < iArgc))
        {
            load_key(pState, pArgv[iArg+1]);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-skip"))
        {
            pOptions->bSkip = TRUE;
        }
        if (!strcmp(pArgv[iArg], "-ncrt"))
        {
            pOptions->bNcrt = TRUE;
        }
        if (!strcmp(pArgv[iArg], "-mintest") && ((iArg+1) < iArgc))
        {
            pOptions->iMinTest = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: mintest=%d\n", pOptions->iMinTest);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-maxtest") && ((iArg+1) < iArgc))
        {
            pOptions->iMaxTest = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: maxtest=%d\n", pOptions->iMaxTest);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-minvers") && ((iArg+1) < iArgc))
        {
            pOptions->iMinVers = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: minvers=%d\n", pOptions->iMinVers);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-maxvers") && ((iArg+1) < iArgc))
        {
            pOptions->iMaxVers = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: maxvers=%d\n", pOptions->iMaxVers);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-minciph") && ((iArg+1) < iArgc))
        {
            pOptions->iMinCiph = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: minciph=%d\n", pOptions->iMinCiph);
            iArg += 1;
        }
        if (!strcmp(pArgv[iArg], "-maxciph") && ((iArg+1) < iArgc))
        {
            pOptions->iMaxCiph = strtol(pArgv[iArg+1], NULL, 10);
            ZPrintf("ssltest: maxciph=%d\n", pOptions->iMaxCiph);
            iArg += 1;
        }
    }

    // find servers in argument list
    if (iArg < iArgc)
    {
        pServers = pArgv;
        pOptions->iMinServ = iArg;
        pOptions->iMaxServ = iArgc;
    }
    else // if no servers specified, use default
    {
        pServers = strServerDefault;
        pOptions->iMinServ = 0;
        pOptions->iMaxServ = 1;
    }

    // log params
    ZPrintf("ssltest: params(vers[%d,%d] ciph[%d,%d] test[%d,%d]\n", pOptions->iMinVers, pOptions->iMaxVers, pOptions->iMinCiph, pOptions->iMaxCiph, pOptions->iMinTest, pOptions->iMaxTest);

    // return servers reference to caller
    return(pServers);
}

static void fail_check(ProtoHttpRefT *pHttpRef, int32_t iLen, char *pResultText, int32_t iResultLen)
{
    ProtoSSLAlertDescT AlertDesc;
    int32_t iSockErr = ProtoHttpStatus(pHttpRef, 'serr', NULL, 0);
    int32_t iSslFail = ProtoHttpStatus(pHttpRef, 'essl', NULL, 0);
    int32_t iAlert = ProtoHttpStatus(pHttpRef, 'alrt', &AlertDesc, sizeof(AlertDesc));
    int32_t iOffset;

    if (iAlert > 0)
    {
        iOffset = ds_snzprintf(pResultText, iResultLen, "%s ssl alert %s (%d)", (iAlert == 1) ? "recv" : "sent", AlertDesc.pAlertDesc, AlertDesc.iAlertType);
    }
    else
    {
        iOffset = ds_snzprintf(pResultText, iResultLen, "download failed (err=%d, sockerr=%d sslerr=%d)", iLen, iSockErr, iSslFail);
    }

    if ((iSslFail == PROTOSSL_ERROR_CERT_INVALID) || (iSslFail == PROTOSSL_ERROR_CERT_HOST) ||
        (iSslFail == PROTOSSL_ERROR_CERT_NOTRUST) || (iSslFail == PROTOSSL_ERROR_CERT_REQUEST))
    {
        ProtoSSLCertInfoT CertInfo;
        if (ProtoHttpStatus(pHttpRef, 'cert', &CertInfo, sizeof(CertInfo)) == 0)
        {
            ds_snzprintf(pResultText+iOffset, iResultLen-iOffset, " cert failure (%d): (C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s)", iSslFail,
                CertInfo.Ident.strCountry, CertInfo.Ident.strState, CertInfo.Ident.strCity,
                CertInfo.Ident.strOrg, CertInfo.Ident.strUnit, CertInfo.Ident.strCommon);
        }
        else
        {
            ds_snzprintf(pResultText+iOffset, iResultLen-iOffset, " could not get cert info");
        }
    }
}


/*** Public Functions ******************************************************************/

// dll-friendly DirtyMemAlloc
#if !defined(DIRTYCODE_DLL)
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#else
void *DirtyMemAlloc2(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#endif
{
    return(malloc(iSize));
}

// dll-friendly DirtyMemFree
#if !defined(DIRTYCODE_DLL)
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#else
void DirtyMemFree2(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
#endif
{
    free(pMem);
}

// usage: ssltest [options] <server1 server2 ... serverN>
int main(int32_t argc, const char *argv[])
{
    int32_t iCount, iLen, iState, iTimeout;
    ProtoHttpRefT *pHttpRef;
    char strBuffer[16*1024];
    const char **pServers;
    SsltestOptionsT SsltestOptions, *pOptions = &SsltestOptions;
    SsltestStateT SsltestState, *pState = &SsltestState;

    #if defined(DIRTYCODE_DLL)
    DirtyMemFuncSet(&DirtyMemAlloc2, &DirtyMemFree2);
    #endif

    // hook into zprintf output
    ZPrintfHook(ssltest_zprintf_hook, NULL);

    // clear state and options settings
    ds_memclr(pState, sizeof(*pState));
    ds_memclr(pOptions, sizeof(*pOptions));

    // get options
    pServers = process_args(pState, pOptions, argc, argv);

    // init test state
    pState->iTest = pOptions->iMinTest;
    pState->iServ = pOptions->iMinServ;
    pState->iVers = pOptions->iMinVers;
    pState->iCiph = pOptions->iMinCiph;

    // start dirtysock
    if (!network_startup())
    {
        return(-1);
    }

    // setup http module
    pHttpRef = ProtoHttpCreate(4096);

    // just keep working
    for (iCount = 0, iState = SSLTEST_STAGE_SETUP, iLen = -1, iTimeout = NetTick()-1; ; )
    {
        const SsltestTestT *pTest = &_ssltest_tests[pState->iTest];

        // see if its time to query
        if ((iTimeout != 0) && (NetTickDiff(NetTick(), iTimeout) >= 0))
        {
            // set up for test
            if (iState == SSLTEST_STAGE_SETUP)
            {
                ZPrintf("ssltest: running %s test\n", pTest->pTestName);
            }
            pTest->pTestFunc(pHttpRef, &SsltestOptions, pState, iState);

            // see if we're at the end of our test
            if (pState->iServ >= pOptions->iMaxServ)
            {
                // go to next test
                pTest = &_ssltest_tests[++pState->iTest];
                // see if we're done with our tests
                if (pState->iTest >= pOptions->iMaxTest)
                {
                    break;
                }
                // reset test state
                pState->iServ = pOptions->iMinServ;
                pState->iVers = pOptions->iMinVers;
                pState->iCiph = pOptions->iMinCiph;
                // move to setup
                iState = SSLTEST_STAGE_SETUP;
                continue;
            }
            else
            {
                iState = SSLTEST_STAGE_RUNNING;
            }

            // if TLS1.0 or TLS1.1, exclude ciphers that were introduced after TLS1.1
            if ((_ssltest_versions[pState->iVers] < PROTOSSL_VERSION_TLS1_2) && (pState->iCiph > SSLTEST_MAXCIPHER_PRE12) && (pState->iCiph != SSLTEST_CIPHER_ALL))
            {
                continue;
            }
            // if TLS1.2, exclude ciphers that were introduced for TLS1.3
            if ((_ssltest_versions[pState->iVers] < PROTOSSL_VERSION_TLS1_3) && (pState->iCiph > SSLTEST_MAXCIPHER_PRE13) && (pState->iCiph != SSLTEST_CIPHER_ALL))
            {
                continue;
            }
            // if TLS1.3, exclude ciphers that predate TLS1.3
            if ((_ssltest_versions[pState->iVers] == PROTOSSL_VERSION_TLS1_3) && (pState->iCiph <= SSLTEST_MAXCIPHER_PRE13) && (pState->iCiph != SSLTEST_CIPHER_ALL))
            {
                continue;
            }

            // set ssl options
            ProtoHttpControl(pHttpRef, 'vers', _ssltest_versions[pState->iVers], 0, NULL);
            ProtoHttpControl(pHttpRef, 'ncrt', pOptions->bNcrt, 0, NULL);

            // set client cert if specified
            if (pState->pClientCert != NULL)
            {
                ProtoHttpControl(pHttpRef, 'scrt', pState->iCertSize, 0, pState->pClientCert);
            }
            // set client key if specified
            if (pState->pClientKey != NULL)
            {
                ProtoHttpControl(pHttpRef, 'skey', pState->iKeySize, 0, pState->pClientKey);
            }

            // set http options
            ProtoHttpControl(pHttpRef, 'keep', 0, 0, NULL);

            // run the test
            iTimeout = NetTick();
            if (!pOptions->bSkip)
            {
                ProtoHttpGet(pHttpRef, pServers[pState->iServ], FALSE);
                iTimeout += 30*1000;
            }
            iLen = 0;
            iCount += 1; // count the attempt
        }

        if (!pOptions->bSkip)
        {
            // update protohttp
            ProtoHttpUpdate(pHttpRef);

            // read incoming data into buffer
            if ((iLen = ProtoHttpRecvAll(pHttpRef, strBuffer, sizeof(strBuffer) - 1)) != PROTOHTTP_RECVWAIT)
            {
                char strFailInfo[256] = "";
                int32_t iVers = pState->iVers;
                if ((iLen > 0) || (iLen == PROTOHTTP_RECVBUFF))
                {
                    pTest->pTestFunc(pHttpRef, &SsltestOptions, &SsltestState, SSLTEST_STAGE_RESULT);
                    ZPrintf("ssltest: [success] version=%s cipher=%s server=%s (%s)\n", _ssltest_tlsnames[iVers], _ssltest_ciphernames[pState->iCiph], pServers[pState->iServ], pState->strResult);
                }
                else
                {
                    fail_check(pHttpRef, iLen, strFailInfo, sizeof(strFailInfo));
                    ZPrintf("ssltest: [failure] version=%s cipher=%s server=%s (%s)\n", _ssltest_tlsnames[iVers], _ssltest_ciphernames[pState->iCiph], pServers[pState->iServ], strFailInfo);
                }
                pTest->pTestFunc(pHttpRef, &SsltestOptions, &SsltestState, SSLTEST_STAGE_COMPLETE);
                iTimeout = NetTick() - 1;
            }
        }
        else
        {
            ZPrintf("ssltest: [skipped] version=%s cipher=%s server=%s\n", _ssltest_tlsnames[pState->iVers], _ssltest_ciphernames[pState->iCiph], pServers[pState->iServ]);
            pTest->pTestFunc(pHttpRef, &SsltestOptions, &SsltestState, SSLTEST_STAGE_COMPLETE);
        }

        // sleep a bit
        NetConnSleep(10);
    }

    ZPrintf("ssltest: done (%d tests)\n", iCount);

    // shut down HTTP
    ProtoHttpDestroy(pHttpRef);

    // disconnect from the network
    NetConnDisconnect();

    // shutdown the network connections && destroy the dirtysock code
    NetConnShutdown(FALSE);
    return(0);
}