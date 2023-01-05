#include "core/stdafx.h"
#include "localize/localize.h"
#include "pluginsystem/modsystem.h"

bool Localize_LoadLocalizationFileLists(CLocalize* thisptr)
{
	v_CLocalize__LoadLocalizationFileLists(thisptr);

	for (auto& mod : g_pModSystem->GetModList())
	{
		if (mod.m_iState == CModSystem::eModState::ENABLED)
		{
			for (auto& it : mod.m_vszLocalizationFiles)
			{
				v_CLocalize__AddFile(thisptr, it.c_str(), NULL);
			}
		}
	}

	DevMsg(eDLL_T::ENGINE, "Loaded localization files.\n");

	return true;
}

void Localize_Attach()
{
	DetourAttach((LPVOID*)&v_CLocalize__LoadLocalizationFileLists, &Localize_LoadLocalizationFileLists);
}

void Localize_Detach()
{
	DetourDetach((LPVOID*)&v_CLocalize__LoadLocalizationFileLists, &Localize_LoadLocalizationFileLists);
}