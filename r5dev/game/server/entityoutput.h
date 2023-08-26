#ifndef ENTITYOUTPUT_H
#define ENTITYOUTPUT_H
#include "variant_t.h"

#define EVENT_FIRE_ALWAYS	-1

class CBaseEntity;


//-----------------------------------------------------------------------------
// Purpose: A COutputEvent consists of an array of these CEventActions. 
//			Each CEventAction holds the information to fire a single input in 
//			a target entity, after a specific delay.
//-----------------------------------------------------------------------------
class CEventAction
{
public:
	int m_type;
	char gap_4[4];

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none

	int m_scriptEnt;
	char gap_24[4];
	char m_scriptFunc[16];

	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	//static int s_iNextIDStamp; !TODO[ AMOS ]: If found, make this a ptr and link it to the one in the game engine!

	CEventAction* m_pNext;
};

//-----------------------------------------------------------------------------
// Purpose: Stores a list of connections to other entities, for data/commands to be
//			communicated along.
//-----------------------------------------------------------------------------
class CBaseEntityOutput
{
public:
	virtual ~CBaseEntityOutput() {};
	virtual int Save(/*ISave*/ __int64 /*save*/) {return 1; /*!!! IMPLEMENTATION IN ENGINE !!!*/}
	virtual int Restore(/*IRestore*/ __int64 /*restore*/, int /*elementCount*/) { return 1; /*!!! IMPLEMENTATION IN ENGINE !!!*/ }

protected:
	variant_t m_Value;
	CEventAction* m_ActionList;
	//DECLARE_SIMPLE_DATADESC();

	CBaseEntityOutput() {} // this class cannot be created, only it's children

private:
	CBaseEntityOutput(CBaseEntityOutput&); // protect from accidental copying
};


//-----------------------------------------------------------------------------
// Purpose: parameterless entity event
//-----------------------------------------------------------------------------
class COutputEvent : public CBaseEntityOutput
{
public:
};

#endif // ENTITYOUTPUT_H
