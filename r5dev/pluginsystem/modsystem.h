#pragma once
#include "vpc/keyvalues.h"
#include <filesystem/filesystem.h>

class CModAppSystemGroup;


class CModSystem
{
public:
	enum eModState : int8_t
	{
		UNLOADED = -1, // loading was unsuccessful (error occurred)
		LOADING = 0,  // if mod is being loaded
		DISABLED = 1, // if disabled by user
		ENABLED  = 2, // if enabled by user and loaded properly
	};

	struct ModInstance_t
	{
		ModInstance_t(const fs::path& basePath);

		inline void SetState(eModState state) { m_iState = state; };

		inline bool IsEnabled() { return m_iState == eModState::ENABLED; };

		string m_szName; // mod display name
		string m_szModID; // internal mod identifier
		fs::path m_BasePath; // path to folder containg all mod files
		string m_szDescription; // mod description
		string m_szVersion; // version string
		KeyValues* m_SettingsKV;

		eModState m_iState = eModState::UNLOADED;

		std::vector<string> m_vszLocalizationFiles;
	};

	void Init();

	// load mod enabled/disabled status from file on disk
	void LoadModStatusList();
	void WriteModStatusList();

	inline vector<ModInstance_t>& GetModList() { return m_vModList; };
	inline std::map<size_t, bool>& GetEnabledList() { return m_vEnabledList; };

private:
	vector<ModInstance_t> m_vModList;
	std::map<size_t, bool> m_vEnabledList;
};
extern CModSystem* g_pModSystem;
                              