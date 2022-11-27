#pragma once

/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
inline CMemory p_AIL_LogFunc;
inline auto v_AIL_LogFunc = p_AIL_LogFunc.RCast<void(*)(int64_t nLogLevel, const char* pszMessage)>();

void MilesCore_Attach();
void MilesCore_Detach();

///////////////////////////////////////////////////////////////////////////////
class MilesCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: AIL_LogFunc                          : {:#18x} |\n", p_AIL_LogFunc.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_AIL_LogFunc = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xDA\x48\x8D\x15\x00\x00\x00\x00"), "xxxxxxxxxxxx????");
		v_AIL_LogFunc = p_AIL_LogFunc.RCast<void(*)(int64_t, const char*)>();
		// 0x1409D1420 // 40 53 48 83 EC 20 48 8B DA 48 8D 15 ? ? ? ? //
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(MilesCore);
