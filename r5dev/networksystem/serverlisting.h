#pragma once


//struct NetGameMod_t
//{
//	string m_svPackage;
//	int m_nNumber;
//	bool m_bRequired;
//	string m_svDownloadLink;
//
//	//NLOHMANN_DEFINE_TYPE_INTRUSIVE(NetGameMod_t, m_svPackage, m_nNumber, m_bRequired, m_svDownloadLink)
//};

struct NetGameServer_t
{
	string m_svHostName;
	string m_svDescription;
	bool m_bHidden;

	string m_svHostMap = "mp_lobby";
	string m_svPlaylist = "dev_default";

	string m_svIpAddress;
	string m_svGamePort; // TODO: should be 'int'.
	string m_svEncryptionKey;

	string m_svRemoteChecksum; // TODO: should be 'unsigned int'.
	string m_svSDKVersion;

	string m_svPlayerCount; // TODO: should be 'int'.
	string m_svMaxPlayers; // TODO: should be 'int'.
	int64_t m_nTimeStamp = -1;

	//vector<NetGameMod_t> m_vMods;
};