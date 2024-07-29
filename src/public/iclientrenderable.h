#ifndef ICLIENTRENDERABLE_H
#define ICLIENTRENDERABLE_H

//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------
typedef unsigned short ClientRenderHandle_t;

enum
{
	INVALID_CLIENT_RENDER_HANDLE = (ClientRenderHandle_t)0xffff,
};

//-----------------------------------------------------------------------------
// Purpose: All client entities must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientRenderable
{
	// Gets at the containing class...
	virtual IClientUnknown* GetIClientUnknown() = 0;
	// TODO:
};

class IClientModelRenderable
{
	// TODO:
	virtual void InterfaceNeedsRebuild() = 0;
};


#endif // ICLIENTRENDERABLE_H