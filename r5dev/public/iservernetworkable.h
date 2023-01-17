//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERNETWORKABLE_H
#define ISERVERNETWORKABLE_H

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseEntity;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class IServerNetworkable
{
	// These functions are handled automatically by the server_class macros and CBaseNetworkable.
public:
	virtual CBaseEntity* GetBaseEntity()  {}; // Only used by game code.
	virtual const char* GetClassName() const {};

protected:
	// Should never call delete on this! 
	virtual					~IServerNetworkable() {}
	virtual					void* Unk() {};
};


#endif // ISERVERNETWORKABLE_H
