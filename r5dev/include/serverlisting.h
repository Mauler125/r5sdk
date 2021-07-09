#pragma once
#include <string>
#include <iostream>

class ServerListing
{
	
public:

	std::string token;
	std::string name;
	std::string version;

	ServerListing(std::string token, std::string name, std::string version);
	bool Select();
};

