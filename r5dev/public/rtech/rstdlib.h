#pragma once

class RHashMap;

/* ==== RTECH =========================================================================================================================================================== */
// [ PIXIE ]: I'm very unsure about this, but it really seems like it
inline int(*v_RHashMap_FindSlot)(RHashMap* const thisptr);
inline void(*v_RHashMap_FreeSlot)(RHashMap* const thisptr, const int slotNum);


class RHashMap
{
public:
	inline int FindSlot(void)
	{
		return v_RHashMap_FindSlot(this);
	}

	inline void FreeSlot(const unsigned int slotNum)
	{
		v_RHashMap_FreeSlot(this, slotNum);
	}

private:
	int m_index;
	int m_slotsLeft;
	int m_structSize;
	int m_searchMask;
	void* m_buffer;
	int m_slotsUsed;
	int padding_perhaps;
};

class RHashMap_MT
{
public:
	inline int FindSlot(void)
	{
		AcquireSRWLockExclusive(&m_lock);
		const int slot = m_mgr.FindSlot();
		ReleaseSRWLockExclusive(&m_lock);

		return slot;
	}

	inline void FreeSlot(const unsigned int slotNum)
	{
		AcquireSRWLockExclusive(&m_lock);
		m_mgr.FreeSlot(slotNum);
		ReleaseSRWLockExclusive(&m_lock);
	}

private:
	RHashMap m_mgr;
	SRWLOCK m_lock;
};

#pragma pack(push, 4)
class RBitRead
{
public:
	FORCEINLINE uint64_t ReadBits(const uint32_t numBits)
	{
		Assert(numBits <= 64, "RBitRead::ReadBits: numBits must be less than or equal to 64.");
		return m_dataBuf & ((1ull << numBits) - 1);
	}

	FORCEINLINE void DiscardBits(const uint32_t numBits)
	{
		Assert(numBits <= 64, "RBitRead::DiscardBits: numBits must be less than or equal to 64.");
		this->m_dataBuf >>= numBits;
		this->m_bitsRemaining += numBits;
	}

	uint64_t m_dataBuf;
	int m_bitsRemaining;
};
#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
class V_ReSTD : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("RHashMap::FindSlot", v_RHashMap_FindSlot);
		LogFunAdr("RHashMap::FreeSlot", v_RHashMap_FreeSlot);
	}
	virtual void GetFun(void) const 
	{
		g_GameDll.FindPatternSIMD("44 8B 51 0C 4C 8B C1").GetPtr(v_RHashMap_FindSlot);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 44 8B 59 0C").GetPtr(v_RHashMap_FreeSlot);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////
