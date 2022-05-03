#pragma once

class CBanSystem
{
public:
	CBanSystem(void);
	void operator[](std::pair<std::string, std::int64_t> pair);
	void Load(void);
	void Save(void) const;

	void AddEntry(std::string svIpAddress, std::int64_t nOriginID);
	void DeleteEntry(std::string svIpAddress, std::int64_t nOriginID);

	void AddConnectionRefuse(std::string svError, std::int64_t nOriginID);
	void DeleteConnectionRefuse(std::int64_t nUserID);

	bool IsBanned(std::string svIpAddress, std::int64_t nOriginID) const;
	bool IsRefuseListValid(void) const;
	bool IsBanListValid(void) const;

	void BanListCheck(void);

private:
	std::vector<std::pair<std::string, std::int64_t>> vsvrefuseList = {};
	std::vector<std::pair<std::string, std::int64_t>> vsvBanList = {};
};

extern CBanSystem* g_pBanSystem;
