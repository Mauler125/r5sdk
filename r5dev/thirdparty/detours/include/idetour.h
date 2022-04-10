#ifndef IDETOUR_H
#define IDETOUR_H

#define ADDDETOUR(x,y) static std::size_t dummy_reg_##y = AddDetour( new x() );
#define XREGISTER(x,y)  ADDDETOUR(x, y)
#define REGISTER(x)     XREGISTER(x, __COUNTER__)

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

class HDetour : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }

	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};

namespace
{
	std::int32_t nPad = 9;
	std::vector<IDetour*> vDetour;
	std::size_t AddDetour(IDetour* pDetour)
	{
		vDetour.push_back(pDetour);
		return vDetour.size();
	}
}

REGISTER(HDetour);
#endif // IDETOUR_H
