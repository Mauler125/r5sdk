#pragma once

class CBanSystem
{
public:
	CBanSystem(void);
	void operator[](std::pair<string, uint64_t> pair);

	void Load(void);
	void Save(void) const;

	void AddEntry(string svIpAddress, uint64_t nOriginID);
	void DeleteEntry(string svIpAddress, uint64_t nOriginID);

	void AddConnectionRefuse(string svError, uint64_t nOriginID);
	void DeleteConnectionRefuse(uint64_t nOriginID);

	void BanListCheck(void);

	bool IsBanned(string svIpAddress, uint64_t nOriginID) const;
	bool IsRefuseListValid(void) const;
	bool IsBanListValid(void) const;

private:
	vector<std::pair<string, uint64_t>> m_vRefuseList = {};
	vector<std::pair<string, uint64_t>> m_vBanList = {};
};

extern CBanSystem* g_pBanSystem;
