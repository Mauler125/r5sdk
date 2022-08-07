#pragma once
// Forward declatartion because errors otherwise
//class CUIBaseSurface;
#include "basepanel.h"

enum ModStatusLevel_t {
	enabledM = 0,
	disabledM = 1,
	invalidM = 2,
};

using json = nlohmann::json;

//-----------------------------------------------------------------------------
// Purpose: custom script run on type
//-----------------------------------------------------------------------------
enum runon {
	server,
	client,
	ui,
};

//-----------------------------------------------------------------------------
// Purpose: class for each script
//-----------------------------------------------------------------------------
class CustomScript {
	public:
		std::string path;
		runon runOn;
		std::string clcb;
		std::string sercb;

		//-----------------------------------------------------------------------------
		// Purpose: constructs each custom script object. each one is added to a vector in modObject
		//-----------------------------------------------------------------------------
		CustomScript(json object);
};

//-----------------------------------------------------------------------------
// Purpose: class for each mod
//-----------------------------------------------------------------------------
class ModObject {
	public:
		bool invalid = false;
		std::string appid = "Placeholder";
		std::string name = "Placeholder";
		std::string description = "Placeholder";
		std::string modJsonLoc = "";
		std::vector<std::string> authors;
		std::vector<std::string> contacts;
		std::string version;
		std::vector<CustomScript> customScripts;
		std::string dumped = "";
		bool toggled;
		json object;
		
		CUIBaseSurface* window;

		ModObject();
		
		//-----------------------------------------------------------------------------
		// Purpose: switch's mod state
		//-----------------------------------------------------------------------------
		void updateJson();

		//-----------------------------------------------------------------------------
		// Purpose: convert object contents to string
		//-----------------------------------------------------------------------------
		std::string string();

		//-----------------------------------------------------------------------------
		// Purpose: constructs mod object by adding name, appid, description, etc. to the object
		//-----------------------------------------------------------------------------
		ModObject(std::string content, std::string modJson, CUIBaseSurface* windowP);
};


//-----------------------------------------------------------------------------
// Purpose: allMods log type
//-----------------------------------------------------------------------------
enum ModType {
	all,
	enabled,
	disabled,
	invalid,
	valid
};

//-----------------------------------------------------------------------------
// Purpose: class to manage mods
//-----------------------------------------------------------------------------
class ModManager {
public:
	std::vector<ModObject> mods;
	std::string scriptRson = "";

	// Add a mod to the list of mods
	ModObject addMod(std::string contents, std::string modJson, CUIBaseSurface* window);
	// Move any mods thats enabled for scripts.rson
	void moveEnabled();
	// Generate scripts.rson
	void generateRson(std::vector<ModObject> compiledMods);
	// Returns a list of the mods name the follows the filter [Default filter is all mods]
	std::vector<ModObject> modsList(ModType modtype = all);
	// Returns the number of mods that follow the filter [Default filter is all mods]
	int modsListNum(ModType modtype = all);
};

//-----------------------------------------------------------------------------
// Purpose: removes compiled_mods directory - ran with atexit
//-----------------------------------------------------------------------------
//void compiledModsRemove() {
//	const std::string compMods = "mods\\compiled_mods";
//	if (fs::exists(compMods))
//		fs::remove_all(compMods);
//}

struct ModManager_t {
	ModManager_t() {}

	ModManager_t(ModStatusLevel_t nLevel, ModObject object)
	{
		m_nLevel = nLevel;
		m_object = object;
	}

	ModStatusLevel_t m_nLevel;
	ModObject m_object;
};