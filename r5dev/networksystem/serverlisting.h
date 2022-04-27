#pragma once

struct ServerListing
{
	std::string svServerName;
	std::string svMapName = "mp_rr_canyonlands_staging";
	std::string svIpAddress;
	std::string svPort;
	std::string svPlaylist = "dev_default";
	bool bHidden{};
	std::string svRemoteChecksum;
	std::string svVersion;
	std::string svEncryptionKey;
};
