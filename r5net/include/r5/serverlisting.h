#pragma once

struct ServerListing
{
	std::string name;
	std::string map;
	std::string ip;
	std::string port;
	std::string playlist;
	bool hidden;
	std::string remoteChecksum;
	std::string version;
	std::string netchanEncryptionKey;
};

