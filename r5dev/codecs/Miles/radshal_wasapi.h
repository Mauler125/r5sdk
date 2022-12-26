#pragma once

/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
inline CMemory p_WASAPI_GetAudioDevice;

///////////////////////////////////////////////////////////////////////////////
class VRadShal : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: WASAPI_GetAudioDevice                : {:#18x} |\n", p_WASAPI_GetAudioDevice.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_WASAPI_GetAudioDevice = g_RadAudioSystemDll.FindPatternSIMD("48 8B C4 48 89 58 20 55 56 41 54");
		// 0x18005AD10 // 48 8B C4 48 89 58 20 55 56 41 54 //
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRadShal);
