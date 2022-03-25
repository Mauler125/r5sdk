#include "core/stdafx.h"
#include "client/client.h"
#include "engine/baseclient.h"

///////////////////////////////////////////////////////////////////////////////
CBaseClient* g_pClient = reinterpret_cast<CBaseClient*>(g_pClientBuffer.GetPtr());
