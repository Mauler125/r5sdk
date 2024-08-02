#ifndef ICLIENTTHINKABLE_H
#define ICLIENTTHINKABLE_H

class IClientThinkable
{
public:
	// Gets at the containing class...
	virtual IClientUnknown* GetIClientUnknown() = 0;
	// TODO:
};

#endif // ICLIENTTHINKABLE_H