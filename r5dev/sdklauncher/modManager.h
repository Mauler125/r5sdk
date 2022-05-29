#pragma once

#include "../thirdparty/nlohmann/json.hpp"

using json = nlohmann::json;

class modObject;

//-----------------------------------------------------------------------------
// Purpose: class for each script
//-----------------------------------------------------------------------------
class customScript {
	public:
		std::string path;
		std::string runon;
		std::string clcb;
		std::string sercb;

		//-----------------------------------------------------------------------------
		// Purpose: constructs each custom script object. each one is added to a vector in modObject
		//-----------------------------------------------------------------------------
		customScript(json object) {
			path = object["Path"];
			runon = object["RunOn"];

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
		std::string modJsonLoc;
		std::vector<std::string> authors;
		std::vector<std::string> contacts;
		std::string version;
		std::vector<customScript> customScripts;
		std::string dumped = "";
		bool toggled = true;
		json object;

		//-----------------------------------------------------------------------------
		// Purpose: switch's mod state
		//-----------------------------------------------------------------------------
		void switchStates() {
			toggled = !toggled;

			object["enabled"] = toggled;

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
							version = object["Description"].get<std::string>();
						}
						if (object.contains("CustomScripts")) {
							if (toggled) {
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

	void addMod(std::string contents, std::string modJson) {
		modObject object(contents, modJson);
		mods.push_back(object);
	}

	std::string modsList(modType modtype = modType::all) {
		std::string string;

		switch (modtype) {
		case modType::all:
			for (auto& i : mods) {
				string.append(i.name + " ");
			}
			break;
		case modType::enabled:
			for (auto& i : mods) {
				if (i.toggled) {
					string.append(i.name + " ");
				}
			}
			break;
		case modType::disabled:
			for (auto& i : mods) {
				if (!i.toggled) {
					string.append(i.name + " ");
				}
			}
			break;
		case modType::invalid:
			for (auto& i : mods) {
				if (i.invalid) {
					string.append(i.name + " ");
				}
			}
			break;
		case modType::valid:
			for (auto& i : mods) {
				if (!i.invalid) {
					string.append(i.name + " ");
				}
			}
			break;
		}

		return string;
	}
};