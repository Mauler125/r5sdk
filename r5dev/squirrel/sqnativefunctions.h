#pragma once
#include "squirrel/sqapi.h"

namespace SQNativeFunctions
{
	namespace IBrowser
	{
        SQRESULT GetServerName(void* sqvm);
        SQRESULT GetServerPlaylist(void* sqvm);
        SQRESULT GetServerMap(void* sqvm);
        SQRESULT GetServerAmount(void* sqvm);
        SQRESULT GetSDKVersion(void* sqvm);
        SQRESULT GetPromoData(void* sqvm);
        SQRESULT SetEncKeyAndConnect(void* sqvm);
        SQRESULT CreateServerFromMenu(void* sqvm);
        SQRESULT JoinPrivateServerFromMenu(void* sqvm);
        SQRESULT GetPrivateServerMessage(void* sqvm);
        SQRESULT ConnectToIPFromMenu(void* sqvm);
	}
}