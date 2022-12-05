#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "public/imaterialsystem.h"
#include "engine/matsys_interface.h"

//-----------------------------------------------------------------------------
// Updates the material system config
//-----------------------------------------------------------------------------
bool UpdateCurrentVideoConfig(MaterialSystem_Config_t* pConfig)
{
	// This is a bug in Respawn's engine.
	// In 'OverrideMaterialSystemConfigFromCommandLine',
	// They added a cmdline check for '-noborder' and '-forceborder'.
	// However, '-noborder' and 'forceborder' perform the exact same operation.
	// Both feature the or instruction, there was unfortunately not
	// enough bytes left to assemble a 'not and' operation to mitigate 
	// this bug without code caves, which is why we create this hook to remove the
	// flag after it has been ran in 'OverrideMaterialSystemConfigFromCommandLine',
	// but before 'UpdateCurrentVideoConfig' so that the file gets created properly.
	if (CommandLine()->FindParm("-forceborder"))
	{
		pConfig->m_Flags &= ~3U;
	}

	return v_UpdateCurrentVideoConfig(pConfig);
}

///////////////////////////////////////////////////////////////////////////////
void MatSys_Iface_Attach()
{
	DetourAttach(&v_UpdateCurrentVideoConfig, &UpdateCurrentVideoConfig);
}

void MatSys_Iface_Detach()
{
	DetourDetach(&v_UpdateCurrentVideoConfig, &UpdateCurrentVideoConfig);
}