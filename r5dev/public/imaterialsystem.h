#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

//-----------------------------------------------------------------------------
// Video mode info..
//-----------------------------------------------------------------------------
struct MaterialVideoMode_t
{
	int m_Width;
	int m_Height;
	int m_Format;
	int m_RefreshRate;
};

//-----------------------------------------------------------------------------
// Material system config..
//-----------------------------------------------------------------------------
struct MaterialSystem_Config_t
{
	MaterialVideoMode_t m_VideoMode;
	int m_nPad;
	int m_Flags;

	// TODO: The rest..
};

#endif // IMATERIALSYSTEM_H
