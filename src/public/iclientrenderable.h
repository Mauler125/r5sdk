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

class IClientRenderable
{
	void* __vftable /*VFT*/;
};

class IClientModelRenderable
{
	void* __vftable /*VFT*/;
};


#endif // ICLIENTRENDERABLE_H