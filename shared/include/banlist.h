#pragma once

class BanList
{
public:
	BanList()
	{
		Load();
	}

	void operator[](std::pair<std::string, std::int64_t> pair)
	{
		AddEntry(pair.first, pair.second);
	}

	void Load()
	{
		std::filesystem::path path = std::filesystem::current_path() /= "banlist.config"; // Get current path + banlist.config

		nlohmann::json in;
		std::ifstream banFile(path, std::ios::in); // Parse ban list.

		int totalBans = 0;

		if (banFile.good() && banFile) // Check if it parsed.
		{
			banFile >> in; // into json.
			banFile.close(); // Close file.

			if (!in.is_null()) // Check if json is valid
			{
				if (!in["totalBans"].is_null()) // Is the totalBans field populated?
				{
					totalBans = in["totalBans"].get<int>(); // Get the totalBans field.
				}
			}

			for (int i = 0; i < totalBans; i++) // Loop through total bans.
			{
				nlohmann::json entry = in[std::to_string(i).c_str()]; // Get Entry for current ban.
				if (entry.is_null()) // Check if entry is valid.
					continue;

				std::int64_t originID = entry["originID"].get<std::int64_t>(); // Get originID field from entry.
				std::string ipAddress = entry["ipAddress"].get<std::string>(); // Get ipAddress field from entry.

				banList.push_back(std::make_pair(ipAddress, originID)); // Push back into vector.
			}
		}
	}

	void Save()
	{
		nlohmann::json out;

		for (int i = 0; i < banList.size(); i++)
		{
			out["totalBans"] = banList.size(); // Populate totalBans field.
			out[std::to_string(i).c_str()]["ipAddress"] = banList[i].first; // Populate ipAddress field for this entry.
			out[std::to_string(i).c_str()]["originID"] = banList[i].second; // Populate originID field for this entry.
		}

		std::filesystem::path path = std::filesystem::current_path() /= "banlist.config"; // Get current path + banlist.config
		std::ofstream outFile(path, std::ios::out | std::ios::trunc); // Write config file..

		outFile << out.dump(4); // Dump it into config file..
		outFile.close(); // Close the file handle.
	}

	void AddEntry(std::string ipAddress, std::int64_t originID)
	{
		if (!ipAddress.empty() && originID > 0) // Check if args are valid.
		{
			banList.push_back(std::make_pair(ipAddress, originID)); // Push it back into the vector.
		}
	}

	void DeleteEntry(std::string ipAddress, std::int64_t originID)
	{
		for (int i = 0; i < banList.size(); i++) // Loop through vector.
		{
			if (ipAddress.compare(banList[i].first) == NULL || originID == banList[i].second) // Do any entries match our vector?
			{
				banList.erase(banList.begin() + i); // If so erase that vector element.
			}
		}
	}

	void AddConnectionRefuse(std::string error, int userID)
	{
		if (refuseList.empty())
		{
			refuseList.push_back(std::make_pair(error, userID));
		}
		else
		{
			for (int i = 0; i < refuseList.size(); i++) // Loop through vector.
			{
				if (refuseList[i].second != userID) // Do any entries match our vector?
				{
					refuseList.push_back(std::make_pair(error, userID)); // Push it back into the vector.
				}
			}
		}
	}

	void DeleteConnectionRefuse(int userID)
	{
		for (int i = 0; i < refuseList.size(); i++) // Loop through vector.
		{
			if (refuseList[i].second == userID) // Do any entries match our vector?
			{
				refuseList.erase(refuseList.begin() + i); // If so erase that vector element.
			}
		}
	}

	bool IsBanned(std::string ipAddress, std::int64_t originID)
	{
		for (int i = 0; i < banList.size(); i++)
		{
			std::string ip = banList[i].first; // Get first pair entry.
			std::int64_t origin = banList[i].second; // Get second pair entry.

			if (ip.empty()) // Check if ip is empty.
				continue;

			if (origin <= 0) // Is originID below 0?
				continue;

			if (ip.compare(ipAddress) == NULL) // Do they match?
				return true;

			if (originID == origin) // Do they match?
				return true;
		}

		return false;
	}

	bool IsRefuseListValid()
	{
		return !refuseList.empty();
	}

	bool IsBanListValid()
	{
		return !banList.empty();
	}

	std::vector<std::pair<std::string, int>> refuseList = {};;
private:
	std::vector<std::pair<std::string, std::int64_t>> banList = {};
};
