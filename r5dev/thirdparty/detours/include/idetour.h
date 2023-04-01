#ifndef IDETOUR_H
#define IDETOUR_H

#define ADDDETOUR(x,y) static std::size_t dummy_reg_##y = AddDetour( new x(), #x );
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

class VDetour : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }

	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};

inline static std::vector<IDetour*> g_DetourVector;
inline static std::unordered_set<IDetour*> g_DetourSet;
inline std::size_t AddDetour(IDetour* pDetour, const char* pszName)
{
	IDetour* pVFTable = reinterpret_cast<IDetour**>(pDetour)[0];
	auto p = g_DetourSet.insert(pVFTable); // Only register if VFTable isn't already registered.

	assert(p.second); // Code bug: duplicate registration!!! (called 'REGISTER(...)' from a header file?).
	p.second ? g_DetourVector.push_back(pDetour) : delete pDetour;

	return g_DetourVector.size();
}

REGISTER(VDetour);
#endif // IDETOUR_H
