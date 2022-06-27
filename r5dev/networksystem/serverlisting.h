#pragma once


struct NetGameMod_t
{
	string m_svPackage;
	int m_nNumber;
	bool m_bRequired;
	string m_svDownloadLink;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(NetGameMod_t, m_svPackage, m_nNumber, m_bRequired, m_svDownloadLink)
};

struct NetGameServer_t
{
	string svServerName;
	string svMapName = "mp_lobby";
	string svPlaylist = "dev_default";
	string svIpAddress;
	string svPort;
	bool bHidden{};
	string svRemoteChecksum;
	string svVersion;
	string svEncryptionKey;
};
