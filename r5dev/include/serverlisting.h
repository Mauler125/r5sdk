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

	ServerListing(std::string name, std::string map, std::string ip, std::string version);
	bool Select();
};

