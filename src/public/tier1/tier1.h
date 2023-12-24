//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//
#ifndef TIER1_H
#define TIER1_H
#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------
template< class IInterface, int ConVarFlag = 0 > 
class CTier1AppSystem : public CTier0AppSystem< IInterface >
{
public:
	virtual bool Connect( const CreateInterfaceFn factory ) = 0;
	virtual void Disconnect( ) = 0;
	virtual void* QueryInterface( const char* const pInterfaceName ) = 0;
	virtual InitReturnVal_t Init( ) = 0;
	virtual void Shutdown( ) = 0;
	virtual AppSystemTier_t GetTier( ) = 0;
	virtual void Reconnect( const CreateInterfaceFn factory, const char* const pInterfaceName ) = 0;
};

#endif // TIER1_H
