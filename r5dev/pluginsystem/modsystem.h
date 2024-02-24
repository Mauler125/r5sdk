#pragma once

#include "tier1/keyvalues.h"
#include "rtech/rson.h"
#include "filesystem/filesystem.h"
#include "vscript/ivscript.h"

#define MOD_STATUS_LIST_FILE "mods.vdf"
#define MOD_SETTINGS_FILE "mod.vdf"
#define MOD_BASE_DIRECTORY "mods"

class CModAppSystemGroup;

class CModSystem
{
public:
	enum eModState : int8_t
	{
		UNLOADED = -1, // loading was unsuccessful (error occurred)
		LOADING,       // if mod is being loaded
		LOADED,        // if a mod has been loaded
		DISABLED,      // if disabled by user
		ENABLED,       // if enabled by user and loaded properly
	};

	struct ModInstance_t
	{
		ModInstance_t(const CUtlString& basePath);
		~ModInstance_t();

		bool ParseSettings();
		//void ParseConVars();
		void ParseLocalizationFiles();

		inline void SetState(eModState state) { m_iState = state; };

		inline bool IsLoaded() const { return m_iState == eModState::LOADED; };
		inline bool IsEnabled() const { return m_iState == eModState::ENABLED; };

		inline const CUtlString& GetBasePath() const { return m_BasePath; };
		inline CUtlString GetScriptCompileListPath() const { return m_BasePath + GAME_SCRIPT_COMPILELIST; };

		KeyValues* GetRequiredSettingsKey(const char* settingsPath, const char* key) const;

		inline RSON::Node_t* LoadScriptCompileList() const
		{
			return RSON::LoadFromFile(GetScriptCompileListPath().Get(), "PLATFORM");
		};

		KeyValues* m_SettingsKV;
		eModState m_iState = eModState::UNLOADED;
		bool m_bHasScriptCompileList; // if this mod has a scripts.rson file that exists

		CUtlVector<CUtlString> m_LocalizationFiles;

		CUtlString m_Name;
		CUtlString m_ModID;
		CUtlString m_Description;
		CUtlString m_Version;

		CUtlString m_BasePath;
	};

	~CModSystem();

	void Init();

	// load mod enabled/disabled status from file on disk
	void UpdateModStatusList();
	void LoadModStatusList(CUtlMap<CUtlString, bool>& enabledList);
	void WriteModStatusList();

	const inline CUtlVector<ModInstance_t*>& GetModList() { return m_ModList; };

private:
	CUtlVector<ModInstance_t*> m_ModList;
};

extern CModSystem g_ModSystem;

FORCEINLINE CModSystem* ModSystem()
{
	return &g_ModSystem;
}
