#include "manager.h"

using namespace nlohmann;

modManager::modManager() {
	refresh();
}

modManager::modManager(modManager* manager) {
	// Copy constructor
	mods = manager->mods;
	modsfolder = manager->modsfolder;
}

void modManager::refresh() {
	// Get the mods folder
	for (const auto& entry : std::filesystem::directory_iterator(modsfolder)) {
		// For each mod folder, create a mod object
		// Then add it to the mods array
		mod* newmod = new mod;
		newmod->location.folder = entry.path();
		newmod->location.scripts = entry.path() / "scripts";
		newmod->location.jason = entry.path() / "mod.json";
		
		// Check if the mod.json file exists
		if (!std::filesystem::exists(newmod->location.jason)) {
			newmod->mod_status = mod::invalid;
			continue;
		}

		// Read mod.json file, then put it through the json library
		std::ifstream modjson(newmod->location.jason);
		modjson >> newmod->modjsondata;
		json* modjsondata = &newmod->modjsondata;
		// Check if mod contains required modjson files
		// This includes appid and name;
		if (!modjsondata->contains("appid") || !modjsondata->contains("name")) {
			newmod->mod_status = mod::invalid;
			continue;
		}

		// If it has a custom scripts field, and it's longer than 0, then add the entries to the customscripts vector
		if (modjsondata->contains("customscripts") && modjsondata->at("customscripts").size() > 0) {
			for (auto& script : modjsondata->at("customscripts")) {
				customScript newscript;
				newscript.path = std::filesystem::path(script.at("path").get<std::string>());
				if (script.at("type") == "ui") {
					newscript.scripttype = customScript::type::ui;
				}
				else if (script.at("type") == "client") {
					newscript.scripttype = customScript::type::client;
				}
				else if (script.at("type") == "server") {
					newscript.scripttype = customScript::type::server;
				}
				else {
					newscript.scripttype = customScript::type::server;
				}
				newmod->customscripts.push_back(newscript);
			}
		}

		// Update it so the correct fields exist
		newmod->update();

		mods.push_back(newmod);
	}
}