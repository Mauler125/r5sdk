#pragma once

#include "../thirdparty/nlohmann/json.hpp"

using json = nlohmann::json;

class modObject;

std::vector<modObject> mods;

//-----------------------------------------------------------------------------
// Purpose: class for each script
//-----------------------------------------------------------------------------
class customScript {
	public:
		std::string path;
		std::string runon;
		std::string clcb;
		std::string sercb;


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
		std::string appid = "Placeholder";
		std::string name = "Placeholder";
		std::string description = "Placeholder";
		std::vector<std::string> authors;
		std::vector<std::string> contacts;
		std::string version;
		std::vector<customScript> customScripts;
		std::string dumped = "";
		bool toggled = true;
		json object;

		modObject(std::string content) {
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
					}
				}
				catch (json::parse_error& e) {
					/// TODO: Tell the user it failed to parse
				}
			}
		};
};