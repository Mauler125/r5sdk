#ifndef ICVAR_H
#define ICVAR_H

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConCommandBase;

//-----------------------------------------------------------------------------
// ConVars/ComCommands are marked as having a particular DLL identifier
//-----------------------------------------------------------------------------
typedef int CVarDLLIdentifier_t;

//-----------------------------------------------------------------------------
// ConVars/ComCommands are marked as having a particular DLL identifier
//-----------------------------------------------------------------------------
typedef int CVarDLLIdentifier_t;

abstract_class ICVarIteratorInternal
{
public:
	virtual void            SetFirst(void) = 0;
	virtual void            Next(void)     = 0;
	virtual bool            IsValid(void)  = 0;
	virtual ConCommandBase* Get(void)      = 0;
};

#endif // ICVAR_H
