#include "core/stdafx.h"
#include "modManager.h"

using json = nlohmann::json;

CustomScript::CustomScript(json object) {
	path = object["Path"];
	if (object["RunOn"] == "UI")
		runOn = runon::ui;
	if (object["RunOn"] == "Client")
		runOn = runon::client;
	if (object["RunOn"] == "Server")
		runOn = runon::server;

	if (object.contains("ClientCallback"))
		clcb = object["ClientCallback"];
	if (object.contains("ServerCallback"))
		sercb = object["ServerCallback"];
}

void ModObject::updateJson() {
	object["enabled"] = toggled;

	modJsonLoc += "\\mod.json";

	if (fs::exists(this->modJsonLoc)) {
		std::ofstream stream(this->modJsonLoc, std::ofstream::trunc);
		if (stream.good()) {
			stream << object.dump(1, 9);
		}
	}
}

std::string ModObject::string() {

	std::string string;
	string.append(appid + " " + name + " " + description + " ");
	for (auto& a : authors)
		string.append(a + " ");
	for (auto& c : contacts)
		string.append(c + " ");
	string.append(version + " ");
	if (toggled == true)
		string.append("true ");
	if (toggled == false)
		string.append("false ");

	return string;
}

ModObject::ModObject() {};

ModObject::ModObject(std::string content, std::string modJson, CUIBaseSurface* windowP) {
	modJsonLoc = modJson;
	window = windowP;	
	if (!content.empty()) {
		try {
			object = json::parse(content);

			if (object.contains("Name") && object.contains("AppId") && object.contains("Description")) {
				if (!object["Name"].empty() || !object["AppId"].empty() || !object["Description"].empty()) {
					name = object["Name"].get<std::string>();
					appid = object["AppId"].get<std::string>();
					description = object["Description"].get<std::string>();
					if (!object["enabled"].empty()) {
						toggled = object["enabled"];
					}
				}

				if (object.contains("Authors")) {
					for (auto& a : object["Authors"]) {
						authors.push_back(a);
					}
				}
				if (object.contains("Contacts")) {
					for (auto& c : object["Contacts"]) {
						contacts.push_back(c);
					}
				}
				if (object.contains("Version")) {
					version = object["Version"].get<std::string>();
				}
				if (object.contains("CustomScripts")) {
					for (auto& i : object["CustomScripts"]) {
						dumped = i.dump();
						if (i.contains("Path") && i.contains("RunOn")) {
							CustomScript script(i);
							customScripts.push_back(script);
						}
						else {
							continue;
						}
					}
				}
			}
			else {
				// TODO: Something messed up message
				invalid = true;
				window->logText(spdlog::level::level_enum::err, "Error. Invalid mod.json for mod: " + modJsonLoc);
			}
		}
		catch (json::parse_error& e) {
			/// TODO: Tell the user it failed to parse
			invalid = true;
			window->logText(spdlog::level::level_enum::err, "Error. Invalid mod.json for mod: " + modJsonLoc);
		}
	}
};

ModObject ModManager::addMod(std::string contents, std::string modJson, CUIBaseSurface* windowPP) {
	ModObject object(contents, modJson, windowPP);
	mods.push_back(object);
	return object;
}

void ModManager::moveEnabled() {
	std::vector<ModObject> compiledMods;
	const std::string compMods = "mods\\compiled_mods";
	const auto copyOptions = fs::copy_options::recursive | fs::copy_options::overwrite_existing;

	if (fs::exists(compMods))
		fs::remove_all(compMods);
	if (!fs::exists(compMods))
		fs::create_directory(compMods);

	for (auto& mLoaded : mods) {
		if (mLoaded.toggled) {
			// Should probably check for duplicates instead of overwiting
			fs::copy(mLoaded.modJsonLoc, compMods, copyOptions);
			compiledMods.push_back(mLoaded);
		}
	}
	fs::remove("mods\\compiled_mods\\mod.json");

	generateRson(compiledMods);
}

void ModManager::generateRson(std::vector<ModObject> compiledMods) {
	std::vector<CustomScript> serverRun;
	std::vector<CustomScript> clientRun;
	std::vector<CustomScript> uiRun;

	for (auto& compMod : compiledMods) {
		for (auto& script : compMod.customScripts) {
			switch (script.runOn) {
			case server:
				serverRun.push_back(script);
				break;
			case client:
				clientRun.push_back(script);
				break;
			case ui:
				uiRun.push_back(script);
				break;
			}
		}


		// Server addition
		scriptRson.append("\n\"When:\" Server\nScripts:\n[");
		for (auto& script : serverRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");

		// Client addition
		scriptRson.append("\n\"When:\" Client\nScripts:\n[");
		for (auto& script : clientRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");

		// UI addition
		scriptRson.append("\n\"When:\" UI\nScripts:\n[");
		for (auto& script : uiRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");


		if (fs::exists("mods\\compiled_mods\\scripts\\vscripts")) {
			fs::create_directory("mods\\compiled_mods\\scripts\\vscripts");
			std::ofstream stream("mods\\compiled_mods\\scripts\\vscripts\\scripts.rson");

			stream << scriptRson;
			stream.close();
		}
	}
}

std::vector<ModObject> ModManager::modsList(ModType modtype) {
	std::vector<ModObject> modObjectList;

	switch (modtype) {
	case all:
		for (auto& i : mods) {
			modObjectList.push_back(i);
		}
		break;
	case enabled:
		for (auto& i : mods) {
			if (i.toggled) {
				modObjectList.push_back(i);
			}
		}
		break;
	case disabled:
		for (auto& i : mods) {
			if (!i.toggled) {
				modObjectList.push_back(i);
			}
		}
		break;
	case invalid:
		for (auto& i : mods) {
			if (i.invalid) {
				modObjectList.push_back(i);
			}
		}
		break;
	case valid:
		for (auto& i : mods) {
			if (!i.invalid) {
				modObjectList.push_back(i);
			}
		}
		break;
	}

	return modObjectList;
}

int ModManager::modsListNum(ModType modtype) {
	int number = 0;

	switch (modtype) {
	case all:
		number = mods.size();
		break;
	case enabled:
		for (auto& i : mods) {
			if (i.toggled) {
				number++;
			}
		}
		break;
	case disabled:
		for (auto& i : mods) {
			if (!i.toggled) {
				number++;
			}
		}
		break;
	case invalid:
		for (auto& i : mods) {
			if (i.invalid) {
				number++;
			}
		}
		break;
	case valid:
		for (auto& i : mods) {
			if (!i.invalid) {
				number++;
			}
		}
		break;
	}

	return number;
}