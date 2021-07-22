#pragma once

class ServerListing
{
public:
	ServerListing(std::string name, std::string map, std::string ip, std::string port) : name(name), map(map), ip(ip), port(port)
	{
		// for future constructor use.
	}

	void Select();

	std::string name;
	std::string map;
	std::string ip;
	std::string port;
};

