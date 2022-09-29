#include "core/stdafx.h"

enum class scriptType {
	ui,
	client,
	server
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
	std::vector<std::pair<scriptType, fs::path>> customscripts;
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
private:
	std::vector<std::pair<fs::path, fs::path>> specialFiles{
		{"rpaks", "paks\\Win64"},
		{"vpk", "vpk"},
		{"plugins", "toDo"},
		{"playlist", "platform"}
	};

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
					std::pair<scriptType, fs::path> newscript;
					if (!script.contains("path")) continue;
					newscript.second = fs::path(script.at("path").get<std::string>());
					if (script.at("runon") == "UI") {
						newscript.first = scriptType::ui;
					}
					else if (script.at("runon") == "CLIENT") {
						newscript.first = scriptType::client;
					}
					else if (script.at("runon") == "SERVER") {
						newscript.first = scriptType::server;
					}
					else {
						newscript.first = scriptType::server;
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

		std::vector<fs::path> uiCustomScripts;
		std::vector<fs::path> clCustomScripts;
		std::vector<fs::path> seCustomScripts;

		// Get all the enabled mods
		std::vector<mod*> enabledmods = filterMods(mod::status::enabled);
		for (mod* mod : enabledmods) {
			// Surface level iterate the mod folder to check for any special files
			for (const auto& entry : fs::directory_iterator(mod->location.folder)) {
				// Check if the file is a special file
				for (auto& specialfile : specialFiles) {
					if (entry.path() != specialfile.first) continue;
					// Copy the contents of the folder to the second part of the pair
					fs::copy(entry.path(), builtfolder / specialfile.second, fs::copy_options::recursive);
				}
			}

			// Copy files from the mod's scripts folder to the built folder
			for (const auto& entry : fs::recursive_directory_iterator(mod->location.scripts)) {
				if (entry.is_directory()) continue;

				// Check to see if the file already exists. 
				if (fs::exists(builtfolder / entry)) continue;

				// Copy the file
				fs::copy(entry, builtfolder / entry);
			}

			// Iterate through the custom scripts			
			for (auto& script : mod->customscripts) {
				switch (script.first) {
				case scriptType::server:
					uiCustomScripts.push_back({ script.second });
					break;
				case scriptType::client:
					clCustomScripts.push_back({ script.second });
					break;
				case scriptType::ui:
					seCustomScripts.push_back({ script.second });
					break;
				}
				
			}
		}

		// For each entry in the customScripts vector, build the custom scripts file
		// Format:
		// WHEN: "<type>"
		// Scripts:
		// [
		//		<fileLocation minus vscripts>
		// ]
		std::stringstream rsonFile;
		if (uiCustomScripts.size() != 0)
			rsonFile << "WHEN: \"UI\"" << std::endl
			<< "Scripts:" << std::endl
			<< "[" << std::endl;
		for (auto& cs : uiCustomScripts)
			rsonFile << cs.string() << std::endl;
		if (uiCustomScripts.size() != 0)
			rsonFile << "]" << std::endl;


		if (clCustomScripts.size() != 0)
			rsonFile << "WHEN: \"CLIENT\"" << std::endl
			<< "Scripts:" << std::endl
			<< "[" << std::endl;
		for (auto& cs : clCustomScripts)
			rsonFile << cs.string() << std::endl;
		if (clCustomScripts.size() != 0)
			rsonFile << "]" << std::endl;


		if (seCustomScripts.size() != 0)
			rsonFile << "WHEN: \"SERVER\"" << std::endl
			<< "Scripts:" << std::endl
			<< "[" << std::endl;
		for (auto& cs : seCustomScripts)
			rsonFile << cs.string() << std::endl;
		if (seCustomScripts.size() != 0)
			rsonFile << "]" << std::endl;

		if (seCustomScripts.size() == 0 && clCustomScripts.size() == 0 && uiCustomScripts.size() == 0) return true;
		// Add the current .rson file onto the end for compabiltiiblity
		if (fs::exists("platform\\scripts\\vscripts\\scripts.rson")) {
			std::ifstream rsonFileC("platform\\scripts\\vscripts\\scripts.rson");
			rsonFile << rsonFileC.rdbuf();
			rsonFileC.close();
		}

		// Write the generated RSON to the built folders vscript folder
		fs::path vscriptfolder = builtfolder / "vscripts";
		if (!fs::exists(vscriptfolder))
			fs::create_directory(vscriptfolder);
		std::ofstream generatedRSONfile(vscriptfolder / "scripts.rson");
		generatedRSONfile << rsonFile.str();

		return true;
	}
};

modManager* g_ModManager = new modManager();