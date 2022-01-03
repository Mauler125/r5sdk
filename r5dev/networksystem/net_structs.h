#pragma once

namespace R5Net {

	struct NetGameMod
	{
		std::string package;
		int number;
		bool requiredForClients;
		std::string downloadLink;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(NetGameMod, package, number, requiredForClients, downloadLink)

	};

	struct NetGameServer
	{
		std::string name;
		std::string description;
		std::string password;

		int playerCount;
		int maxPlayerCount;
		std::string playlist;
		std::string mapName;

		std::vector<NetGameMod> mods;

		std::string ipAddress;
		int gamePort;
		std::string encryptionKey;
		std::string remoteChecksum;

		std::string reloadedVersion;

		std::string publicRef;

		int lastPing = -1;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(NetGameServer, name, description, password, playerCount, maxPlayerCount, playlist, mapName, mods, ipAddress, gamePort, encryptionKey, remoteChecksum, reloadedVersion, publicRef)
	};


	////// Requests
	struct UpdateGameServerMSRequest
	{
		NetGameServer gameServer;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateGameServerMSRequest, gameServer)
	};

	struct GetIsUserBannedMSRequest
	{
		int oid;
		std::string ipAddress;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetIsUserBannedMSRequest, oid, ipAddress)
	};

	struct GetPrivateGameServerInfoMSRequest
	{
		std::string publicRef;
		std::string password;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetPrivateGameServerInfoMSRequest, publicRef, password)
	};


	/// <summary>
	/// Responses
	/// </summary>
	/// 
	/// 

	enum class EResponseStatus
	{
		NO_REACH,
		SUCCESS,
		FORBIDDEN,
		NOT_FOUND,
		MS_ERROR,
		UNKNOWN
	};


	struct DefaultMSResponse
	{
		EResponseStatus status = EResponseStatus::NO_REACH;
		std::string error;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(DefaultMSResponse, error)
	};

	struct GetGlobalStatsMSResponse : DefaultMSResponse
	{
		int noPlayers;
		int noServers;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetGlobalStatsMSResponse, noPlayers, noServers)
	};

	struct GetGameServersListMSResponse : DefaultMSResponse
	{
		std::vector<NetGameServer> publicServers;
		std::vector<NetGameServer> privateServers;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetGameServersListMSResponse, publicServers, privateServers)
	};

	struct UpdateGameServerMSResponse : DefaultMSResponse
	{
		NetGameServer gameServer;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(UpdateGameServerMSResponse, gameServer)
	};

	struct GetIsUserBannedMSResponse : DefaultMSResponse
	{
		bool isBanned;
		std::string metaString;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetIsUserBannedMSResponse, isBanned, metaString)
	};

	struct GetPrivateGameServerInfoMSResponse : DefaultMSResponse
	{
		NetGameServer gameServer;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(GetPrivateGameServerInfoMSResponse, gameServer)
	};

}
