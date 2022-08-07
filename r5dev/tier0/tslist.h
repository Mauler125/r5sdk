#ifndef TSLIST_H
#define TSLIST_H

inline CMemory g_pMallocPool;

void TSList_Attach();
void TSList_Detach();
///////////////////////////////////////////////////////////////////////////////
class VTSListBase : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pMallocPool                        : {:#18x} |\n", g_pMallocPool.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pMallocPool = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"),
			"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????").Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pMallocPool = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9"),
			"xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx").Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VTSListBase);

#endif // TSLIST_H
