/*H**********************************************************
 * secure.c
 *
 * Description:
 *
 * Test SSL code.
 *
 * Copyright (c) Tiburon Entertainment, Inc. 2002.
 *               All rights reserved.
 *
 * Ver.     Description
 * 1.0      03/08/2002 (GWS) [Greg Schaefer] Cleanup and revision
 *
 *H*/

// Includes

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/protossl.h"

#include "libsample/zlib.h"

#include "testermodules.h"


// Constants

typedef struct SecureRef      // module state storage
{
    uint32_t timeout;       // operation timeout

    ProtoSSLRefT *ssl;
    char url[256];

} SecureRef;


/*F******************************************************
 * CmdSecureCB
 *
 * Description:
 *   Callback to wait for authent completion.
 *
 * Inputs:
 *   Context, arg count, arg strings (zlib standard)
 *
 * Outputs:
 *   Exit code
 * 
 * Ver. Description
 * 1.0  10/4/99 (GWS) Initial version
 *
 *F*/
static int32_t _CmdSecureCB(ZContext *argz, int32_t argc, char *argv[])
{
    SecureRef *ref = (SecureRef *)argz;

    // check for kill
    if (argc == 0)
    {
        ProtoSSLDestroy(ref->ssl);
        return(0);
    }

    // check for timeout
    if (ZTick() > ref->timeout)
    {
        ZPrintf("%s: timeout\n", argv[0]);
        ProtoSSLDestroy(ref->ssl);
        return(-1);
    }

    // give transaction time
    ProtoSSLUpdate(ref->ssl);

    // see if connection complete
    if (ProtoSSLStat(ref->ssl, 'stat', NULL, 0) > 0)
    {
        int32_t len;
        char buf[1024];

        if (ref->url[0] != 0)
        {
            ProtoSSLSend(ref->ssl, ref->url, -1);
            ref->url[0] = 0;
        }

        len = ProtoSSLRecv(ref->ssl, buf, sizeof(buf));
        if (len > 0)
        {
            ZPrintf("[%s]\n", buf);
        }
    }

    // keep running
    return(ZCallback(&_CmdSecureCB, 100));
}

/*F******************************************************
 * CmdSecure
 *
 * Description:
 *   Test the SSL handler
 *
 * Inputs:
 *   Context, arg count, arg strings (zlib standard)
 *
 * Outputs:
 *   Exit code
 * 
 * Ver. Description
 * 1.0  10/4/99 (GWS) Initial version
 *
 *F*/
int32_t CmdSecure(ZContext *argz, int32_t argc, char *argv[])
{
    SecureRef *ref;

    // handle help
    if (argc < 3)
    {
        ZPrintf("   test protossl module\n");
        ZPrintf("   usage: %s [address] [port] - setup SSL connection\n", argv[0]);
        return(0);
    }

    // setup connection
    ZPrintf("%s: ssl connect\n", argv[0]);

    ref = (SecureRef *) ZContextCreate(sizeof(*ref));
    ref->timeout = ZTick()+120*1000;

    // setup ssl state
    ref->ssl = ProtoSSLCreate();

    // attempt to connect
    ProtoSSLConnect(ref->ssl, TRUE, argv[1], 0, strtol(argv[2], NULL, 10));

    // setup the url
    strcpy(ref->url, "GET / HTTP/1.0\r\n"
            "Accept: */*\r\n"
            "Content-Length: 0\r\n"
            "User-Agent: Custom/1.0\r\n"
            "\r\n");
    
    // wait for reply
    return(ZCallback(&_CmdSecureCB, 100));
}