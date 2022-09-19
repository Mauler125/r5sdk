#ifndef ICONCOMMAND_H
#define ICONCOMMAND_H
#include "icvar.h"

//-----------------------------------------------------------------------------
// Abstract interface for ConVars
//-----------------------------------------------------------------------------
abstract_class IConCommandBase
{
public:
	virtual						~IConCommandBase(void) = 0;

	virtual	bool				IsCommand(void) const = 0;

	virtual bool				IsFlagSet(int flag) const = 0; // Check flag
	virtual void				AddFlags(int flags) = 0;       // Set flag
	virtual void				RemoveFlags(int flags) = 0;    // Clear flag
	virtual int					GetFlags() const = 0;          // Get flag

	virtual const char* GetName(void) const = 0;               // Return name of cvar
	virtual const char* GetHelpText(void) const = 0;           // Return help text for cvar
	virtual const char* GetUsageText(void) const = 0;          // Return usage text for cvar
	virtual void SetAccessor(char* bAccessors) const = 0;

	virtual bool				IsRegistered(void) const = 0;
	virtual CVarDLLIdentifier_t	GetDLLIdentifier() const = 0;  // Returns the DLL identifier

protected:
	virtual void				Create(const char* pName, const char* pHelpString = 0,
									int flags = 0) = 0;
	virtual void				Init() = 0; // Used internally by OneTimeInit to initialize/shutdown
};


#endif // ICONCOMMAND_H