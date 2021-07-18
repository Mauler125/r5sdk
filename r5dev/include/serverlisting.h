#pragma once
#include <string>
#include <iostream>

class ServerListing
{
public:

	std::string name;
	std::string map;
	std::string ip;
	std::string version;
	int expiry;

	ServerListing(std::string name, std::string map, std::string ip, std::string version, int expiry);
	void Select();
};

