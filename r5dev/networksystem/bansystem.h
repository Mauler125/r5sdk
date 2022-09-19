#pragma once

class CBanSystem
{
public:
	void Load(void);
	void Save(void) const;

	bool AddEntry(const string& svIpAddress, const uint64_t nNucleusID);
	bool DeleteEntry(const string& svIpAddress, const uint64_t nNucleusID);

	bool AddConnectionRefuse(const string& svError, const uint64_t nNucleusID);
	bool DeleteConnectionRefuse(const uint64_t nNucleusID);

	void BanListCheck(void);

	bool IsBanned(const string& svIpAddress, const uint64_t nNucleusID) const;
	bool IsRefuseListValid(void) const;
	bool IsBanListValid(void) const;

	void KickPlayerByName(const string& svPlayerName);
	void KickPlayerById(const string& svHandle);

	void BanPlayerByName(const string& svPlayerName);
	void BanPlayerById(const string& svHandle);

	void UnbanPlayer(const string& svCriteria);

private:
	vector<std::pair<string, uint64_t>> m_vRefuseList = {};
	vector<std::pair<string, uint64_t>> m_vBanList = {};
};

extern CBanSystem* g_pBanSystem;
