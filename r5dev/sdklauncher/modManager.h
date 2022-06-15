#pragma once

#include "../thirdparty/nlohmann/json.hpp"
#include <stdlib.h>

using json = nlohmann::json;

class modObject;

//-----------------------------------------------------------------------------
// Purpose: custom script run on type
//-----------------------------------------------------------------------------
enum runon {
	ui,
	client,
	server
};

//-----------------------------------------------------------------------------
// Purpose: class for each script
//-----------------------------------------------------------------------------
class customScript {
	public:
		std::string path;
		runon runOn;
		std::string clcb;
		std::string sercb;

		//-----------------------------------------------------------------------------
		// Purpose: constructs each custom script object. each one is added to a vector in modObject
		//-----------------------------------------------------------------------------
		customScript(json object) {
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
};

//-----------------------------------------------------------------------------
// Purpose: class for each mod
//-----------------------------------------------------------------------------
class modObject {
	public:
		bool invalid = false;
		std::string appid = "Placeholder";
		std::string name = "Placeholder";
		std::string description = "Placeholder";
		std::string modJsonLoc = "";
		std::vector<std::string> authors;
		std::vector<std::string> contacts;
		std::string version;
		std::vector<customScript> customScripts;
		std::string dumped = "";
		bool toggled;
		json object;

		modObject() {}
		
		//-----------------------------------------------------------------------------
		// Purpose: switch's mod state
		//-----------------------------------------------------------------------------
		void updateJson() {
			object["enabled"] = toggled;

			modJsonLoc += "\\mod.json";

			if (fs::exists(modJsonLoc)) {
				std::ofstream stream(modJsonLoc, std::ofstream::trunc);
				if (stream.good()) {
					stream << object.dump(1, 9);
				}
			}
		}

		//-----------------------------------------------------------------------------
		// Purpose: convert object contents to string
		//-----------------------------------------------------------------------------
		std::string string() {
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

		//-----------------------------------------------------------------------------
		// Purpose: constructs mod object by adding name, appid, description, etc. to the object
		//-----------------------------------------------------------------------------
		modObject(std::string content, std::string modJson) {
			modJsonLoc = modJson;
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
									customScript script(i);
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
					}
				}
				catch (json::parse_error& e) {
					/// TODO: Tell the user it failed to parse
					invalid = true;
				}
			}
		};
};


//-----------------------------------------------------------------------------
// Purpose: allMods log type
//-----------------------------------------------------------------------------
enum modType {
	all,
	enabled,
	disabled,
	invalid,
	valid
};

//-----------------------------------------------------------------------------
// Purpose: class to manage mods
//-----------------------------------------------------------------------------
class modManager {
public:
	std::vector<modObject> mods;
	std::string scriptRson = "";

	modObject addMod(std::string contents, std::string modJson) {
		modObject object(contents, modJson);
		mods.push_back(object);
		return object;
	}

	// Generate a list of mods that are being moved into compiled_mods
	// Then pass that list into generateRson, to generate an rson file for it

	//-----------------------------------------------------------------------------
	// Purpose: copy enabled mods into compiled_mods folder
	//-----------------------------------------------------------------------------
	void moveEnabled() {
		std::vector<modObject> compiledMods;
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

	void generateRson(std::vector<modObject> compiledMods) {
		std::vector<customScript> uiRun;
		std::vector<customScript> clientRun;
		std::vector<customScript> serverRun;

		for (auto& compMod : compiledMods) {
			for (auto& script : compMod.customScripts) {
				switch (script.runOn) {
				case ui:
					uiRun.push_back(script);
					break;
				case client:
					clientRun.push_back(script);
					break;
				case server:
					serverRun.push_back(script);
					break;
				}
			}
		}

		// UI addition
		scriptRson.append("\n\"When:\" UI\nScripts:\n[");
		for (auto& script : uiRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");

		// Client addition
		scriptRson.append("\n\"When:\" Client\nScripts:\n[");
		for (auto& script : clientRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");

		// Server addition
		scriptRson.append("\n\"When:\" Server\nScripts:\n[");
		for (auto& script : serverRun)
			scriptRson.append("\n\t" + script.path);
		scriptRson.append("\n]\n");

		if (fs::exists("mods\\compiled_mods\\scripts\\vscripts")) {
				fs::create_directory("mods\\compiled_mods\\scripts\\vscripts");
				std::ofstream stream("mods\\compiled_mods\\scripts\\vscripts\\scripts.rson");

				stream << scriptRson;
				stream.close();
		}
	}

	std::vector<modObject> modsList(modType modtype = modType::all) {
		std::vector<modObject> modObjectList;

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

	int modsListNum(modType modtype = modType::all) {
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
};

//-----------------------------------------------------------------------------
// Purpose: removes compiled_mods directory - ran with atexit
//-----------------------------------------------------------------------------
//void compiledModsRemove() {
//	const std::string compMods = "mods\\compiled_mods";
//	if (fs::exists(compMods))
//		fs::remove_all(compMods);
//}