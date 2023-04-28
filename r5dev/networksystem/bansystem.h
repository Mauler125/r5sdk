#pragma once

typedef vector<std::pair<string, uint64_t>> BannedVec_t;

class CBanSystem
{
public:
	void Load(void);
	void Save(void) const;

	bool AddEntry(const string& svIpAddress, const uint64_t nNucleusID);
	bool DeleteEntry(const string& svIpAddress, const uint64_t nNucleusID);

	bool IsBanned(const string& svIpAddress, const uint64_t nNucleusID) const;
	bool IsBanListValid(void) const;

	void KickPlayerByName(const char* playerName, const char* reason = nullptr);
	void KickPlayerById(const char* playerHandle, const char* reason = nullptr);

	void BanPlayerByName(const char* playerName, const char* reason = nullptr);
	void BanPlayerById(const char* playerHandle, const char* reason = nullptr);

	void UnbanPlayer(const string& svCriteria);

private:
	void AuthorPlayerByName(const char* playerName, const bool bBan, const char* reason = nullptr);
	void AuthorPlayerById(const char* playerHandle, const bool bBan, const char* reason = nullptr);

	BannedVec_t m_vBanList;
};

extern CBanSystem* g_pBanSystem;
