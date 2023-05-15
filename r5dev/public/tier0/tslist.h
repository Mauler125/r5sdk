#ifndef TSLIST_H
#define TSLIST_H

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CAlignedMemAlloc
{
public:
	void* Alloc(size_t nSize, size_t nAlignment = 0);
	void Free(void* pMem);

private:
	void* m_pAllocCallback;
	void* m_pFreeCallback;
};

extern CAlignedMemAlloc* g_pAlignedMemAlloc;

//-----------------------------------------------------------------------------
// Singleton aligned memalloc
//-----------------------------------------------------------------------------
inline CAlignedMemAlloc* AlignedMemAlloc()
{
	return g_pAlignedMemAlloc;
}

///////////////////////////////////////////////////////////////////////////////
class VTSListBase : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_AlignedMemAlloc", reinterpret_cast<uintptr_t>(g_pAlignedMemAlloc));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pAlignedMemAlloc = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8B D9 FF 15 ?? ?? ?? ??")
			.Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CAlignedMemAlloc*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pAlignedMemAlloc = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9")
			.Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CAlignedMemAlloc*>();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // TSLIST_H
