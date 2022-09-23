// Alternate mod manager for .r5mod files
#include "core/stdafx.h"
#include "thirdparty/cryptopp/include/sha.h"
#include "thirdparty/cryptopp/include/zdeflate.h"
#include "thirdparty/cryptopp/include/zinflate.h"
#include "thirdparty/cryptopp/include/hex.h"

// Function to read a string - strings are done in the following format:
// 2 bytes - 16 bit unsigned integer - length of string
// n bytes - string data
// Input: *ifstream - file to read from
char* readString(ifstream* file) {
	// Read the length of the string
	uint16_t length;
	file->read((char*)&length, 2);

	// Read the string data
	char* data = new char[length];
	file->read(data, length);

	data[length] = '\0';

	// Return the data
	return data;
}

// Function to write a string - strings are done in the following format:
// 2 bytes - 16 bit unsigned integer - length of string
// n bytes - string data
// Input: *ofstream - file to write to
//		  string - string to write - automagically trims null terminating character
void writeString(ofstream* file, string str) {
	// Get the length
	unsigned int length = str.length();
	// Write the length
	file->write(reinterpret_cast<const char*>(&length), 2);
	// Write the string
	file->write(str.c_str(), length);
}

enum class scriptType {
	ui,
	client,
	server
};

class mod {
public:
	enum states {
		invalid,
		valid,
		unknown
	};

	states state = unknown;
	std::string appid = "";
	std::string displayName = "";
	std::string intName = "";
	std::string desc = "";
	std::string author = "";
	std::string version = "";
	std::string hash = "";
	string sdkVer = "";
	bool modEnabled = true;
	int fileCount = 0;

	bool status() {
		return modEnabled;
	}
	
	bool status(bool state) {
		modEnabled = state;
		return modEnabled;
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
	std::vector<fs::path> tempFiles;
	std::vector<fs::path> conflicFiles;
	
	void refresh() {
		mods.clear();

		// Check for the existance of the mods folder
		if (!fs::exists(modsfolder)) {
			fs::create_directory(modsfolder);
			return;
		}

		for (auto& e : fs::directory_iterator(modsfolder)) {
			if (e.path().extension() != ".r5mod") continue;

			mod* modO = new mod();

			// Open the mod file for reading
			std::ifstream modFile(e.path());
			if (!modFile.is_open()) continue;

			// Check the first three bytes for the header "R5R"
			char* header = new char[3];
			modFile.read(header, 3);
			if (header[0] != 'R' || header[1] != '5' || header[2] != 'R') {
				delete[] header;
				modO->state = mod::states::invalid;
				continue;
			}

			// Read the SDK version
			char* sdkVer = readString(&modFile);
			// Get the hash
			char* hash = new char[21];
			modFile.read(hash, 20);
			hash[20] = '\0';
			// Get the appid
			char* appid = new char[17];
			modFile.read(appid, 16);
			appid[16] = '\0';
			// Get the other mod name
			char* displayName = readString(&modFile);
			// Get the mod name
			char* intName = readString(&modFile);
			// Mod Version
			char* ver = readString(&modFile);
			// Mod description
			char* description = readString(&modFile);
			// Author name
			char* author = readString(&modFile);
			// Get dependancy count
			char* depCountChar = new char[1];
			modFile.read(depCountChar, 1);
			unsigned int depCountInt = *reinterpret_cast<unsigned int*>(depCountChar);
			// Deal with reading deps later
			// Get file count
			char* fileCountR = new char[4];
			modFile.read(fileCountR, 4);
			unsigned int fileCountInt = *reinterpret_cast<unsigned int*>(fileCountR);
			// Get bytes remaining
			char* bytesRemaining = new char[4];
			modFile.read(bytesRemaining, 4);
			unsigned int bytesRemainingInt = *reinterpret_cast<unsigned int*>(bytesRemaining);

			modFile.close();

			// Set the appropriate values in the mod object
			modO->appid = appid;
			modO->displayName = displayName;
			modO->intName = intName;
			modO->desc = description;
			modO->version = ver;
			modO->hash = hash;
			modO->sdkVer = sdkVer;
			modO->author = author;
			modO->fileCount = fileCountInt;

			modO->state = mod::states::valid;

			mods.push_back(modO);
		}
	};

	std::vector<mod*> filterMods(mod::states filter = mod::states::unknown) {
		// Returns a vector of mods with a particular status - default is enabled
		std::vector<mod*> filteredmods;
		for (mod* mod : mods) {
			if (mod->state == filter) 
				filteredmods.push_back(mod);
		}
		return filteredmods;
	};
	
	std::vector<mod*> enabledMods() {
		std::vector<mod*> filteredmods;
		for (mod* mod : mods) {
			if (mod->modEnabled)
				filteredmods.push_back(mod);
		}
		return filteredmods;
	};

	bool updateEnabled() {
		// Updates enabled.json with correct values

		// Build json object for writing array
		nlohmann::json enabled = nlohmann::json::array();
		
		for (mod* mod : mods) {
			if (mod->status())
				enabled.push_back(mod->intName);
		}

		std::string enabledstr = enabled.dump(4);

		// Open enabled.json for writing
		std::ofstream enabledFile(modsfolder.string() + "/enabled.json");
		if (!enabledFile.is_open()) return false;
		enabledFile.seekp(0);
		enabledFile << enabledstr;
		enabledFile.close();
	}

	bool buildMods() {
		// Builds the mods folder
		// First, clear the mods folder
		if (fs::exists(modsfolder)) {
			for (auto& e : fs::directory_iterator(modsfolder)) {
				if (e.path().extension() == ".r5mod") continue;
				fs::remove_all(e.path());
			}
		}

		std::vector<fs::path> uiCustomScripts;
		std::vector<fs::path> clCustomScripts;
		std::vector<fs::path> seCustomScripts;

		// Now, build the mods
		for (mod* mod : enabledMods()) {
			// Open the mod file for reading
			std::ifstream modFile(modsfolder.string() + "/" + mod->intName + ".r5mod", std::ios::binary);
			if (!modFile.is_open()) continue;

			// Check the first three bytes for the header "R5R"
			char* header = new char[3];
			modFile.read(header, 3);
			if (header[0] != 'R' || header[1] != '5' || header[2] != 'R') {
				spdlog::error("Invalid Header");
				return false;
			}
			
			// Read the SDK version
			char* sdkVer = readString(&modFile);
			// Get the hash
			char* hash = new char[21];
			modFile.read(hash, 20);
			hash[20] = '\0';
			// Get the appid
			char* appid = new char[17];
			modFile.read(appid, 16);
			appid[16] = '\0';
			// Get the other mod name
			char* displayName = readString(&modFile);
			// Get the mod name
			char* intName = readString(&modFile);
			// Mod Version
			char* ver = readString(&modFile);
			// Mod description
			char* description = readString(&modFile);
			// Author name
			char* author = readString(&modFile);
			// Get dependancy count
			char* depCountChar = new char[1];
			modFile.read(depCountChar, 1);
			unsigned int depCountInt = *reinterpret_cast<unsigned int*>(depCountChar);
			// Deal with reading deps later
			// Get file count
			char* fileCountR = new char[4];
			modFile.read(fileCountR, 4);
			unsigned int fileCountInt = *reinterpret_cast<unsigned int*>(fileCountR);
			// Get bytes remaining
			char* bytesRemaining = new char[4];
			modFile.read(bytesRemaining, 4);
			unsigned int bytesRemainingInt = *reinterpret_cast<unsigned int*>(bytesRemaining);

			CryptoPP::SHA1 sha1;

			for (int i = 0; i < fileCountInt; i++) {
				// Get the file name
				char* fileName = readString(&modFile);
				// Get the uncompressed file size
				char* uFileSize = new char[4];
				modFile.read(uFileSize, 4);
				unsigned int uFileSizeInt = *reinterpret_cast<unsigned int*>(uFileSize);
				// Get the compressed file size
				char* cFileSize = new char[4];
				modFile.read(cFileSize, 4);
				unsigned int cFileSizeInt = *reinterpret_cast<unsigned int*>(cFileSize);
				// Get custom script state
				char* customScript = new char[2];
				modFile.read(customScript, 1);
				customScript[1] = '\0';

				if (customScript == reinterpret_cast<const char*>("1"))
					uiCustomScripts.push_back({ fs::path(fileName) });
				else if (customScript == reinterpret_cast<const char*>("2"))
					clCustomScripts.push_back({ fs::path(fileName) });
				else if (customScript == reinterpret_cast<const char*>("3"))
					seCustomScripts.push_back({ fs::path(fileName) });

				// Get the file data
				char* fileData = new char[cFileSizeInt];
				modFile.read(fileData, cFileSizeInt);

				// Add to the hash
				sha1.Update(reinterpret_cast<const CryptoPP::byte*>(fileData), cFileSizeInt);

				string decompressedData;
				CryptoPP::Inflator inflator(new CryptoPP::StringSink(decompressedData));
				inflator.Put((const CryptoPP::byte*)fileData, cFileSizeInt);
				inflator.MessageEnd();

				bool specialFileB = false;
				
				// Check to see if the file is located in a "special" folder
				fs::path filePath(fileName);
				for (auto& e : specialFiles) {
					// Check to see if the parent path of the file is equal to the first value
					if (filePath.parent_path() == e.first) {
						// Create the directory to be sure
						fs::create_directories(e.second);
						// If it is, then we need to check to see if the file is in the second value
						// If it is, then we need to write the file to the special folder
						fs::path specialPath = e.second / filePath.filename();
						std::ofstream specialFile(specialPath, std::ios::binary);
						specialFile.write(decompressedData.c_str(), decompressedData.size());
						specialFile.close();
						specialFileB = true;
						break;
					}
				}

				if (specialFileB) continue;

				// Write the file normally to modsFolder / built / filePath
				fs::path outputPath = modsfolder / "built" / filePath;
				fs::create_directories(outputPath.parent_path());
				std::ofstream outputedFile(outputPath, std::ios::binary);
				outputedFile.write(decompressedData.c_str(), decompressedData.size());
				outputedFile.close();
				break;
			}

			// Calculate the hash
			string digest;
			digest.resize(sha1.DigestSize() / 2);
			// Data being hashed is the compressed data
			sha1.TruncatedFinal((CryptoPP::byte*)&digest[0], digest.size());

			string hashString;
			CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hashString));
			CryptoPP::StringSource(digest, true, new CryptoPP::Redirector(encoder));
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
		return true;
	}
	
	modManager() {
		refresh();
	}
};

modManager* g_ModManager = new modManager();