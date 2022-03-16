#pragma once
#ifndef DEDICATED
#include "launcher/IApplication.h"
#endif // !DEDICATED
#include "public/include/globalvars_base.h"
#include "engine/sv_main.h"


//-----------------------------------------------------------------------------
// Purpose: Defines the ways that a map can be loaded.
// Note   : Only seems to get set and checked on 0 and 2: [r5apex_ds.exe + d55990d] for more details.
//-----------------------------------------------------------------------------
enum MapLoadType_t
{
	MapLoad_NewGame    = 0,
	MapLoad_Background = 2
};

//-----------------------------------------------------------------------------
// Purpose: Global variables shared between the engine and the game .dll
//-----------------------------------------------------------------------------
class CGlobalVars : public CGlobalVarsBase
{
public:
	// Current map
	const char*     m_pszMapName;
	int             m_nMapVersion;
	char            m_pad0[4];
	const char*     m_pszStartSpot;   // Seems empty at all times.
	MapLoadType_t   m_eLoadType;      // How the current map was loaded.
	bool            m_bMapLoadFailed; // Map has failed to load, we need to kick back to the main menu (unused?).

	void*           m_pUnk0;          // r5apex_ds.exe 'CBaseServer::Clear() + 0x7E'
	void*           m_pUnk1;          // r5apex_ds.exe 'CBaseServer::Clear() + 0x93'
	void*           m_pUnk2;          // r5apex_ds.exe 'CServer::FrameJob()  + 0x20'
	void*           m_pUnk3;
}; // Size 0x0098

namespace
{
	CGlobalVars* g_ServerGlobalVariables = p_SV_InitGameDLL.Offset(0x0).FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVars*>();
#ifndef DEDICATED
	CGlobalVarsBase* g_ClientGlobalVariables = p_CModAppSystemGroup_Create.Offset(0x0).FindPatternSelf("4C 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 8000).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVarsBase*>();
#endif // !DEDICATED
}

///////////////////////////////////////////////////////////////////////////////
class HEdict : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_ServerGlobalVariables              : 0x" << std::hex << std::uppercase << g_ServerGlobalVariables << std::setw(0) << " |" << std::endl;
#ifndef DEDICATED
		std::cout << "| VAR: g_ClientGlobalVariables              : 0x" << std::hex << std::uppercase << g_ClientGlobalVariables << std::setw(0) << " |" << std::endl;
#endif // !DEDICATED
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HEdict);
