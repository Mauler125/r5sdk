#include "core/stdafx.h"

class customScript {
	// Class for each custom script entry in the mod.json file
	// Holds the scripts location - including its name, and the script type - UI, Client, Server
public:
	enum class type {
		ui,
		client,
		server
	};

	fs::path path;
	type scripttype;
};

class mod {
	// Class for information relating to the mod
	// This includes location, status, appid, etc

public:
	enum status {
		invalid,
		valid,
		enabled,
		disabled,
		unknown
	};
	
	struct locations {
		fs::path folder = "";
		fs::path scripts = "";
		fs::path jason = "";
	};

	status mod_status = unknown;
	std::string mod_appid = "";
	std::string mod_name = "";
	std::string mod_desc = "";
	nlohmann::json modjsondata;
	std::vector<std::string> authors;
	locations location = locations();
	bool modEnabled = false;
	std::vector<customScript> customscripts;
	std::string version = "1.0.0";
	
	bool update() {
		// Updates the mods mod.json file with the correct state

		// Check for the existance of the file
		if (!fs::exists(location.jason))
			return false;

		modjsondata["enabled"] = modEnabled;

		// Write the file
		std::ofstream modjsonwrite(location.jason);
		modjsonwrite << modjsondata.dump(4);
		return true;
	}
	bool toggle() {
		// Toggles the mods state - updates the mod.json file
		modEnabled = !modEnabled;
		return toggle(modEnabled);
	}
	bool toggle(bool value) {
		modEnabled = value;
		update();
		return value;
	}
};

class modManager {
public:
	std::vector<mod*> mods;
	fs::path modsfolder = fs::path("mods");

	modManager() {
		refresh();
	}
	void refresh() {
		// Reset the mods vector
		mods.clear();

		// Get the mods folder
		for (const auto& entry : fs::directory_iterator(modsfolder)) {
			std::cout << "Found Folder: " << entry.path() << std::endl;
			// For each mod folder, create a mod object
			// Then add it to the mods array
			mod* newmod = new mod;
			newmod->location.folder = entry.path();
			newmod->location.scripts = entry.path() / "scripts";
			newmod->location.jason = entry.path() / "mod.json";

			// Check if the mod.json file exists
			if (!fs::exists(newmod->location.jason)) {
				newmod->mod_status = mod::invalid;
				std::cout << "Mod is invalid. Mod.json doesn't exist" << std::endl;
				continue;
			}

			// Read mod.json file, then put it through the json library
			std::ifstream modjson(newmod->location.jason);
			modjson >> newmod->modjsondata;
			nlohmann::json* modjsondata = &newmod->modjsondata;
			// Check if mod contains required modjson files
			// This includes appid and name;
			if (!modjsondata->contains("appid") || !modjsondata->contains("name")) {
				newmod->mod_status = mod::invalid;
				std::cout << "Mod is invalid. Missing appid or name" << std::endl;
				continue;
			}

			// Set the mod names
			newmod->mod_appid = (*modjsondata)["appid"];
			newmod->mod_name = (*modjsondata)["name"];
			newmod->mod_desc = (*modjsondata)["description"];
			newmod->modEnabled = (*modjsondata)["enabled"];
			newmod->authors = (*modjsondata)["authors"];
			newmod->version = (*modjsondata)["version"];
			newmod->mod_status = newmod->modEnabled ? mod::status::enabled : mod::status::disabled;

			// If it has a custom scripts field, and it's longer than 0, then add the entries to the customscripts vector
			if (modjsondata->contains("customscripts")) {
				for (auto& script : modjsondata->at("customscripts")) {
					customScript newscript;
					if (!script.contains("path")) continue;
					newscript.path = fs::path(script.at("path").get<std::string>());
					if (script.at("runon") == "UI") {
						newscript.scripttype = customScript::type::ui;
					}
					else if (script.at("runon") == "CLIENT") {
						newscript.scripttype = customScript::type::client;
					}
					else if (script.at("runon") == "SERVER") {
						newscript.scripttype = customScript::type::server;
					}
					else {
						newscript.scripttype = customScript::type::server;
					}
					newmod->customscripts.push_back(newscript);
				}
			}

			// Update it so the correct fields exist
			// newmod->update();

			mods.push_back(newmod);
		}
	}
	std::vector<mod*> filterMods(mod::status filter = mod::status::enabled) {
		// Returns a vector of mods with a particular status - default is enabled
		std::vector<mod*> filteredmods;
		for (mod* mod : mods) {
			if (mod->mod_status == filter) {
				filteredmods.push_back(mod);
			}
		}
		return filteredmods;
	};
	bool buildMods() {
		// Builds the mods folder, located at mods/built. This includes the RSON and any extra files
		// Returns true if successful, false if not
		fs::path builtfolder = modsfolder / "built";
		if (!fs::exists(builtfolder))
			fs::create_directory(builtfolder);

		std::stringstream generatedRSON;
		// Todo: See if using a custom RSON ignores the other RSON, or if they both get merged together.
		// If they don't. Maybe copy the existing one and add new content on top.

		// Get all the enabled mods
		std::vector<mod*> enabledmods = filterMods(mod::status::enabled);
		for (mod* mod : enabledmods) {
			// For each enabled mod, copy the scripts folder to the built folder
			fs::copy(mod->location.scripts, builtfolder, fs::copy_options::recursive);

			// Build the correct RSON text - example below:
			// When: " <Server/Client/UI> "
			// Scripts:
			// [
			//		" <script path> "
			// ]
			// Then add it to the generated RSON
			for (auto& script : mod->customscripts) {
				std::string scripttype = "";
				switch (script.scripttype) {
				case customScript::type::server:
					scripttype = "SERVER";
					break;
				case customScript::type::client:
					scripttype = "CLIENT";
					break;
				case customScript::type::ui:
					scripttype = "UI";
					break;
				}
				generatedRSON << "When: \"" << scripttype << "\"\nScripts:\n[\n\t\"" << script.path.string() << "\"\n]\n";
			}
		}

		// Write the generated RSON to the built folders vscript folder
		fs::path vscriptfolder = builtfolder / "vscripts";
		if (!fs::exists(vscriptfolder))
			fs::create_directory(vscriptfolder);
		std::ofstream generatedRSONfile(vscriptfolder / "scripts.rson");
		generatedRSONfile << generatedRSON.str();

		return true;
	}
};

modManager* g_ModManager = new modManager();