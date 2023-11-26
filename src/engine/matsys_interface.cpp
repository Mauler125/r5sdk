#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "public/imaterialsystem.h"
#include "engine/matsys_interface.h"

// Disabled, as this patch **only** works if we are writing the file.
// See 'resource/patch/patch_r5apex.exe' for more details regarding how
// this bug has been fixed in the engine module.
/*
//-----------------------------------------------------------------------------
// Updates the material system config
//-----------------------------------------------------------------------------
bool UpdateCurrentVideoConfig(MaterialSystem_Config_t* pConfig)
{
	// This is a bug in Respawn's engine. In
	// 'OverrideMaterialSystemConfigFromCommandLine', they added
	// a cmdline check for '-noborder' and '-forceborder'. However,
	// '-noborder' and 'forceborder' perform the exact same operation.
	if (CommandLine()->FindParm("-forceborder"))
	{
		pConfig->m_Flags &= ~2U;
	}

	return v_UpdateCurrentVideoConfig(pConfig);
}
*/

///////////////////////////////////////////////////////////////////////////////
void VMatSys_Interface::Detour(const bool bAttach) const
{
	//DetourSetup(&v_UpdateCurrentVideoConfig, &UpdateCurrentVideoConfig, bAttach);
}
