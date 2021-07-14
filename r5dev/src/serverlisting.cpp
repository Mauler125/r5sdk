#include "serverlisting.h"
#include "overlay.h"
#include "httplib.h"

ServerListing::ServerListing(std::string name, std::string map, std::string ip, std::string version, int expiry)
{
	this->name = name;
	this->map = map;
	this->ip = ip;
	this->version = version;
	this->expiry = expiry;
}

bool ServerListing::Select()
{
	std::stringstream cmd;
	cmd << "connect " << this->ip;
	RunConsoleCommand(cmd.str().c_str());
	return true;
}