#pragma once


struct NetGameMod_t
{
	string m_svPackage;
	int m_nNumber;
	bool m_bRequired;
	string m_svDownloadLink;

	//NLOHMANN_DEFINE_TYPE_INTRUSIVE(NetGameMod_t, m_svPackage, m_nNumber, m_bRequired, m_svDownloadLink)
};

struct NetGameServer_t
{
	string m_svHostName;
	string m_svDescription;

	string m_svPassword;
	bool m_bHidden;

	string m_svMapName = "mp_lobby";
	string m_svPlaylist = "dev_default";

	string m_svIpAddress;
	int m_nGamePort;

	string m_svEncryptionKey;
	string m_svRemoteChecksum;

	string m_svSDKVersion;
	string m_svPublicRef;

	int m_nPlayerCount;
	int m_nMaxPlayers;
	int m_nLastPing = -1;

	//vector<NetGameMod_t> m_vMods;
};