#pragma once
#include <string>
#include <iostream>

class ServerListing
{
public:
	ServerListing(std::string name, std::string map, std::string ip, std::string version) : name(name), map(map), ip(ip), version(version)
	{
		// for future constructor use.
	}

	void Select();

	std::string name;
	std::string map;
	std::string ip;
	std::string version;
};

