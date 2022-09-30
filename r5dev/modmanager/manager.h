#include "core/stdafx.h"
#include "thirdparty/cryptopp/include/gzip.h"
#include "thirdparty/cryptopp/include/files.h"

enum class scriptType {
	ui,
	client,
	server
};

class mod {
	// Class for information relating to the mod
	// This includes m_Locations, status, appid, etc

public:
	enum eStatus {
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

	eStatus mod_status = unknown;
	std::string m_Appid = "";
	std::string m_Name = "";
	std::string m_Desc = "";
	nlohmann::json m_Json;
	std::vector<std::string> m_Authors;
	locations m_Locations = locations();
	bool m_Enabled = false;
	std::vector<std::pair<scriptType, fs::path>> m_CustomScripts;
	std::string m_Version = "1.0.0";
	
	bool update() {
		// Updates the mods mod.json file with the correct state

		// Check for the existance of the file
		if (!fs::exists(m_Locations.jason))
			return false;

		m_Json["enabled"] = m_Enabled;

		// Write the file
		std::ofstream modjsonwrite(m_Locations.jason);
		modjsonwrite << m_Json.dump(4);
		return true;
	}
	bool toggle() {
		// Toggles the mods state - updates the mod.json file
		m_Enabled = !m_Enabled;
		return toggle(m_Enabled);
	}
	bool toggle(bool value) {
		m_Enabled = value;
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

	void decompressZips() {
		// Decompresses all zip files in the mods folder
		for (auto& file : fs::directory_iterator(modsfolder)) {
			if (file.path().extension() == ".zip") {
				// Decompress .zip into the correctly structued folder
				// TODO: Figure out what I need to do...
			}
		}
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
			newmod->m_Locations.folder = entry.path();
			newmod->m_Locations.scripts = entry.path() / "scripts";
			newmod->m_Locations.jason = entry.path() / "mod.json";

			// Check if the mod.json file exists
			if (!fs::exists(newmod->m_Locations.jason)) {
				newmod->mod_status = mod::invalid;
				std::cout << "Mod is invalid. Mod.json doesn't exist" << std::endl;
				continue;
			}

			// Read mod.json file, then put it through the json library
			std::ifstream modjson(newmod->m_Locations.jason);
			modjson >> newmod->m_Json;
			nlohmann::json* m_Json = &newmod->m_Json;
			// Check if mod contains required modjson files
			// This includes appid and name;
			if (!m_Json->contains("appid") || !m_Json->contains("name")) {
				newmod->mod_status = mod::invalid;
				std::cout << "Mod is invalid. Missing appid or name" << std::endl;
				continue;
			}

			// Set the mod names
			newmod->m_Appid = (*m_Json)["appid"];
			newmod->m_Name = (*m_Json)["name"];
			newmod->m_Desc = (*m_Json)["description"];
			newmod->m_Enabled = (*m_Json)["enabled"];
			newmod->m_Authors = (*m_Json)["authors"];
			newmod->m_Version = (*m_Json)["version"];
			newmod->mod_status = newmod->m_Enabled ? mod::eStatus::enabled : mod::eStatus::disabled;

			// If it has a custom scripts field, and it's longer than 0, then add the entries to the m_CustomScripts vector
			if (m_Json->contains("customscripts")) {
				for (auto& script : m_Json->at("customscripts")) {
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
					newmod->m_CustomScripts.push_back(newscript);
				}
			}

			// Update it so the correct fields exist
			// newmod->update();

			mods.push_back(newmod);
		}
	}
	std::vector<mod*> filterMods(mod::eStatus filter = mod::eStatus::enabled) {
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
		std::vector<mod*> enabledmods = filterMods(mod::eStatus::enabled);
		for (mod* mod : enabledmods) {
			// Surface level iterate the mod folder to check for any special files
			for (const auto& entry : fs::directory_iterator(mod->m_Locations.folder)) {
				// Check if the file is a special file
				for (auto& specialfile : specialFiles) {
					if (entry.path() != specialfile.first) continue;
					// Copy the contents of the folder to the second part of the pair
					fs::copy(entry.path(), builtfolder / specialfile.second, fs::copy_options::recursive);
				}
			}

			// Copy files from the mod's scripts folder to the built folder
			for (const auto& entry : fs::recursive_directory_iterator(mod->m_Locations.scripts)) {
				if (entry.is_directory()) continue;

				// Check to see if the file already exists. 
				if (fs::exists(builtfolder / entry)) continue;

				// Copy the file
				fs::copy(entry, builtfolder / entry);
			}

			// Iterate through the custom scripts			
			for (auto& script : mod->m_CustomScripts) {
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

	modManager() {
		refresh();
	}
};

modManager* g_ModManager = new modManager();