//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "core/stdafx.h"
#include "client/vengineclient_impl.h"

//#ifdef GAMEDLL_S3
bool* m_bRestrictServerCommands = reinterpret_cast<bool*>(g_mGameDll.FindString("DevShotGenerator_Init()").FindPatternSelf("88 05", CMemory::Direction::UP).ResolveRelativeAddressSelf(0x2).OffsetSelf(0x2).GetPtr());
//#endif
