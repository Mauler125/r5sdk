#ifndef IDETOUR_H
#define IDETOUR_H

//-----------------------------------------------------------------------------
// Interface class for context hooks
//-----------------------------------------------------------------------------
class IDetour
{
public:
	virtual ~IDetour() { ; }
	virtual void GetAdr(void) const = 0;
	virtual void GetFun(void) const = 0;
	virtual void GetVar(void) const = 0;
	virtual void GetCon(void) const = 0;

	virtual void Attach(void) const = 0;
	virtual void Detach(void) const = 0;
};

extern std::map<const void*, const IDetour*> g_DetourMap;
std::size_t AddDetour(IDetour* pDetour);

#define ADDDETOUR(x,y) static std::size_t dummy_reg_##y = AddDetour( new x() );
#define XREGISTER(x,y)  ADDDETOUR(x, y)
#define REGISTER(x)     XREGISTER(x, __COUNTER__)

#endif // IDETOUR_H
