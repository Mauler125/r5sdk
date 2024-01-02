#pragma once

/* ==== IAPPSYSTEM ============================================================================================================================================== */
///////////////////////////////////////////////////////////////////////////////
class VAppSystem : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
