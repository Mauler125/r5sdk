#include "core/stdafx.h"
#include "tier1/utlvector.h"
#include "localize/localize.h"
#include "pluginsystem/modsystem.h"

bool Localize_LoadLocalizationFileLists(CLocalize* thisptr)
{
	v_CLocalize__LoadLocalizationFileLists(thisptr);

	const CUtlVector<CModSystem::ModInstance_t*>&
		modList = g_pModSystem->GetModList();

	FOR_EACH_VEC(modList, i)
	{
		const CModSystem::ModInstance_t* mod =
			modList.Element(i);

		if (!mod->IsEnabled())
			continue;

		FOR_EACH_VEC(mod->m_LocalizationFiles, j)
		{
			const char* localizationFile = mod->m_LocalizationFiles.Element(j).Get();

			if (!v_CLocalize__AddFile(thisptr, localizationFile, "PLATFORM"))
				Warning(eDLL_T::ENGINE, "Failed to add localization file '%s'\n", localizationFile);
		}
	}

	return true;
}

void VLocalize::Attach() const
{
	DetourAttach((LPVOID*)&v_CLocalize__LoadLocalizationFileLists, &Localize_LoadLocalizationFileLists);
}

void VLocalize::Detach() const
{
	DetourDetach((LPVOID*)&v_CLocalize__LoadLocalizationFileLists, &Localize_LoadLocalizationFileLists);
}