#pragma once

/* ==== CVENGINECLIENT ================================================================================================================================================== */
inline CMemory p_IVEngineClient_CommandExecute;
inline auto IVEngineClient_CommandExecute = p_IVEngineClient_CommandExecute.RCast<void(*)(void* thisptr, const char* pCmd)>();

///////////////////////////////////////////////////////////////////////////////
extern bool* m_bRestrictServerCommands;

///////////////////////////////////////////////////////////////////////////////
class HVEngineClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: IVEngineClient::CommandExecute       : 0x" << std::hex << std::uppercase << p_IVEngineClient_CommandExecute.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: m_bRestrictServerCommands            : 0x" << std::hex << std::uppercase << m_bRestrictServerCommands                << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_IVEngineClient_CommandExecute = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1E\x41\x8B\xD8"), "xxxx?xxxxxxxx????xxx");
		IVEngineClient_CommandExecute = p_IVEngineClient_CommandExecute.RCast<void(*)(void* thisptr, const char* pCmd)>(); /*48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 41 8B D8*/
	}
	virtual void GetVar(void) const
	{
		m_bRestrictServerCommands = reinterpret_cast<bool*>(g_mGameDll.FindString("DevShotGenerator_Init()").FindPatternSelf("88 05", CMemory::Direction::UP).ResolveRelativeAddressSelf(0x2).OffsetSelf(0x2).GetPtr());
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVEngineClient);