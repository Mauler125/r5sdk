#include "serverlisting.h"
#include "overlay.h"
#include "httplib.h"

ServerListing::ServerListing(std::string token, std::string name, std::string version)
{
	this->token = token;
	this->name = name;
	this->version = version;
}

bool ServerListing::Select()
{
	httplib::Client client("http://localhost:80");
	std::stringstream body;

	body << "uid=" << OriginUID << "&servertoken=" << token;
	client.Post("/browse/select", body.str(), "application/x-www-form-urlencoded");
	std::cout << body.str() << "\n";
	return true;
}