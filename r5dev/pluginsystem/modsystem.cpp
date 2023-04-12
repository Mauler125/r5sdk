//=============================================================================//
//
// Purpose: Manage loading mods
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "modsystem.h"
#include "localize/localize.h"
#include "tier1/cvar.h"
#include "vpc/rson.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the mod system
// Input  :
//-----------------------------------------------------------------------------
void CModSystem::Init()
{
	LoadModStatusList();

	CreateDirectories("platform\\mods");

	for (auto& it : fs::directory_iterator("platform\\mods"))
	{
		if (!it.is_directory())
			continue;

		fs::path basePath = it.path();
		DevMsg(eDLL_T::ENGINE, "Found mod at '%s'.\n", basePath.string().c_str());
		fs::path settingsPath = basePath / "mod.vdf";

		if (fs::exists(settingsPath))
		{
			CModSystem::ModInstance_t modInst = CModSystem::ModInstance_t(basePath);
			if (modInst.m_iState != eModState::UNLOADED)
				m_vModList.push_back(modInst);
		}
	}

	WriteModStatusList();
}

void CModSystem::LoadModStatusList()
{
	if (FileSystem()->FileExists("platform/mods.vdf"))
	{
		KeyValues* pModList = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, "platform/mods.vdf", "GAME");

		for (KeyValues* pSubKey = pModList->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
		{
			size_t idHash = std::hash<std::string>{}(std::string(pSubKey->GetName()));
			m_vEnabledList.emplace(idHash, pSubKey->GetBool());
		}
	}
}

void CModSystem::WriteModStatusList()
{
	KeyValues kv = KeyValues("ModList");
	KeyValues* pModListKV = kv.FindKey("ModList", true);

	for (auto& it : m_vModList)
	{
		bool enabled = false;
		if (it.m_iState == eModState::ENABLED)
			enabled = true;

		pModListKV->SetBool(it.m_szModID.c_str(), enabled);
	}

	CUtlBuffer uBuf = CUtlBuffer(0i64, 0, CUtlBuffer::TEXT_BUFFER);

	kv.RecursiveSaveToFile(uBuf, 0);

	FileSystem()->WriteFile("platform/mods.vdf", NULL, uBuf); // NULL instead of "GAME" because otherwise for some reason the file clears itself when the process exits
}


CModSystem::ModInstance_t::ModInstance_t(const fs::path& basePath) : m_szName(std::string()), m_szModID(std::string()), m_BasePath(basePath), m_szDescription(std::string()), m_szVersion(std::string()), m_SettingsKV(nullptr), m_iState(eModState::LOADING)
{
	std::string settingsPath = (m_BasePath / "mod.vdf").string();

	KeyValues* pSettingsKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, settingsPath.c_str(), "GAME");
	m_SettingsKV = pSettingsKV;

	if (!m_SettingsKV)
	{
		SetState(eModState::UNLOADED);
		Error(eDLL_T::ENGINE, NO_ERROR, "Failed to parse mod.vdf for mod at path '%s'\n", m_BasePath.string().c_str());
		return;
	}

	/////////////////////////////
	// "name" "An R5Reloaded Mod"
	// [rexx]: could be optional and have id as fallback
	KeyValues* pName = pSettingsKV->FindKey("name");

	if (!pName)
	{
		SetState(eModState::UNLOADED);
		Error(eDLL_T::ENGINE, NO_ERROR, "Mod settings file '%s' was missing required 'name' field. Skipping mod...\n", settingsPath.c_str());
		return;
	}

	m_szName = pName->GetString();

	/////////////////////////////
	// "version" "1.0.0"
	KeyValues* pVersion = pSettingsKV->FindKey("version");

	if (!pVersion)
	{
		SetState(eModState::UNLOADED);
		Error(eDLL_T::ENGINE, NO_ERROR, "Mod settings file '%s' was missing required 'version' field. Skipping mod...\n", settingsPath.c_str());
		return;
	}

	m_szVersion = pVersion->GetString();

	/////////////////////////////
	// "id" "r5reloaded.TestMod"
	KeyValues* pId = pSettingsKV->FindKey("id");

	if (!pId)
	{
		SetState(eModState::UNLOADED);
		Error(eDLL_T::ENGINE, NO_ERROR, "Mod settings file '%s' was missing required 'id' field. Skipping mod...\n", settingsPath.c_str());
		return;
	}

	m_szModID = pId->GetString();

	/////////////////////////////
	// optional mod description field
	m_szDescription = pSettingsKV->GetString("description");

	size_t idHash = std::hash<std::string>{}(m_szModID);

	auto& enabledList = g_pModSystem->GetEnabledList();
	if (enabledList.count(idHash) == 0)
	{
		DevMsg(eDLL_T::ENGINE, "Mod does not exist in 'mods.vdf'. Enabling...\n");
		SetState(eModState::ENABLED);
	}
	else
	{
		bool bEnable = enabledList[idHash];
		SetState(bEnable ? eModState::ENABLED : eModState::DISABLED);

		DevMsg(eDLL_T::ENGINE, "Mod exists in 'mods.vdf' and is %s.\n", bEnable ? "enabled" : "disabled");
	}

	if (m_iState != eModState::ENABLED)
		return;

	// parse any additional info from mod.vdf

	// add mod folder to search paths so files can be easily loaded from here
	// [rexx]: maybe this isn't ideal as the only way of finding the mod's files, as there may be name clashes in files where the engine
	//         won't really care about the input file name. it may be better to, where possible, request files by file path relative to root (i.e. including platform/mods/{mod}/)
	FileSystem()->AddSearchPath(m_BasePath.string().c_str(), "GAME", SearchPathAdd_t::PATH_ADD_TO_TAIL);

	KeyValues* pLocalizationFiles = pSettingsKV->FindKey("LocalizationFiles");

	if (pLocalizationFiles)
	{
		for (KeyValues* pSubKey = pLocalizationFiles->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
		{
			this->m_vszLocalizationFiles.push_back(pSubKey->GetName());
		}
	}

	KeyValues* pConVars = pSettingsKV->FindKey("ConVars");

	if (pConVars)
	{
		for (KeyValues* pSubKey = pConVars->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
		{
			const char* pszName = pSubKey->GetName();
			const char* pszHelpString = pSubKey->GetString("helpString");
			const char* pszFlagsString = pSubKey->GetString("flags", "NONE");

			KeyValues* pValues = pSubKey->FindKey("Values");

			const char* pszDefaultValue = "0";
			bool bMin = false;
			bool bMax = false;
			float fMin = 0.f;
			float fMax = 0.f;

			if (pValues)
			{
				pszDefaultValue = pValues->GetString("default", "0");

				// minimum cvar value
				if (pValues->FindKey("min"))
				{
					bMin = true; // has min value
					fMin = pValues->GetFloat("min", 0.f);
				}

				// maximum cvar value
				if (pValues->FindKey("max"))
				{
					bMax = true; // has max value
					fMax = pValues->GetFloat("max", 1.f);
				}
			}

			int flags = FCVAR_NONE;
			if (ConVar::ParseFlagString(pszFlagsString, flags, pszName))
				ConVar::StaticCreate(pszName, pszDefaultValue, flags, pszHelpString, bMin, fMin, bMax, fMax, nullptr, nullptr);
		}
	}


	std::string scriptsRsonPath = (m_BasePath / "scripts/vscripts/scripts.rson").string();

	//RSON::Node_t* rson = RSON::LoadFromFile(scriptsRsonPath.c_str());

	//if (rson)
	//	DevMsg(eDLL_T::ENGINE, "mod rson loaded: %p\n", uintptr_t(rson));
};

CModSystem* g_pModSystem = new CModSystem();