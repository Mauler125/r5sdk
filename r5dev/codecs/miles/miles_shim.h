#ifndef MILES_SHIM_H
#define MILES_SHIM_H

class MilesShim : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // MILES_SHIM_H
