#pragma once
#include "public/include/json.hpp"

class CBanSystem
{
public:
	CBanSystem();
	void operator[](std::pair<std::string, std::int64_t> pair);
	void Load();
	void Save();
	void AddEntry(std::string svIpAddress, std::int64_t nOriginID);
	void DeleteEntry(std::string svIpAddress, std::int64_t nOriginID);
	void AddConnectionRefuse(std::string svError, std::int64_t nOriginID);
	void DeleteConnectionRefuse(std::int64_t nUserID);
	bool IsBanned(std::string svIpAddress, std::int64_t nOriginID);
	bool IsRefuseListValid();
	bool IsBanListValid();

	std::vector<std::pair<std::string, std::int64_t>> vsvrefuseList = {};;
private:
	std::vector<std::pair<std::string, std::int64_t>> vsvBanList = {};
};

extern CBanSystem* g_pBanSystem;
