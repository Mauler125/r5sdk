//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( INETMSGHANDLER_H )
#define INETMSGHANDLER_H

struct IConnectionlessPacketHandler
{
	void* __vftable /*VFT*/;
};

struct INetMessageHandler
{
	void* __vftable /*VFT*/;
};

struct IServerMessageHandler : INetMessageHandler
{};

#endif // INETMSGHANDLER_H