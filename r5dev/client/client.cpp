#include "core/stdafx.h"
#include "client/client.h"

///////////////////////////////////////////////////////////////////////////////
CClient* g_pClient =  reinterpret_cast<CClient*>(p_IVEngineServer_PersistenceAvailable.FindPatternSelf("48 8D 0D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
