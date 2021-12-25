#include <vector>

#ifndef IDETOUR_H
#define IDETOUR_H

#define ADDDETOUR(x,y) static size_t dummy_reg_##y = AddDetour( new x() );
#define XREGISTER(x,y)  ADDDETOUR(x, y)
#define REGISTER(x)     XREGISTER(x, __COUNTER__)

class IDetour
{
public:
	virtual ~IDetour() { ; }
	//virtual void attach() = 0;
	//virtual void detach() = 0;
	virtual void debugp() = 0;
};

namespace
{
	std::int32_t npad = 9;
	std::vector<IDetour*> vdetour;
	size_t AddDetour(IDetour* idtr)
	{
		vdetour.push_back(idtr);
		return vdetour.size();
	}
}

class H : public IDetour
{
	virtual void debugp()
	{
		//
	}
};

REGISTER(H);
#endif // IDETOUR_H
