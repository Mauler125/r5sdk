#pragma once

/* ==== COMMON ========================================================================================================================================================== */
inline CMemory p_COM_ExplainDisconnection = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x48\x81\xEC\x00\x00\x00\x00"), "xxxxxxxxxxxxxxxxxx????");
inline auto COM_ExplainDisconnection = p_COM_ExplainDisconnection.RCast<void* (*)(uint64_t level, const char* fmt, ...)>(); /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ? ? ? ?*/

///////////////////////////////////////////////////////////////////////////////
class HCommon : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: COM_ExplainDisconnection             : 0x" << std::hex << std::uppercase << p_COM_ExplainDisconnection.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCommon);
