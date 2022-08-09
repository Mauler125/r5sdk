#pragma once

class CBanSystem
{
public:
	CBanSystem(void);
	void operator[](std::pair<const string&, const uint64_t> pair);

	void Load(void);
	void Save(void) const;

	void AddEntry(const string& svIpAddress, const uint64_t nOriginID);
	void DeleteEntry(const string& svIpAddress, const uint64_t nOriginID);

	void AddConnectionRefuse(const string& svError, const uint64_t nOriginID);
	void DeleteConnectionRefuse(const uint64_t nOriginID);

	void BanListCheck(void);

	bool IsBanned(const string& svIpAddress, const uint64_t nOriginID) const;
	bool IsRefuseListValid(void) const;
	bool IsBanListValid(void) const;

private:
	vector<std::pair<string, uint64_t>> m_vRefuseList = {};
	vector<std::pair<string, uint64_t>> m_vBanList = {};
};

extern CBanSystem* g_pBanSystem;
