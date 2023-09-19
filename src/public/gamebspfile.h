//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Defines game-specific data
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEBSPFILE_H
#define GAMEBSPFILE_H

struct StaticPropLump_t
{
	Vector3D m_Origin;
	Vector3D m_Angles;
	float m_Scale;
	short m_PropType;
	char m_Solid;
	char m_Flags;
	short m_Skin;
	short m_EnvCubemap;
	float m_FadeDist;
	Vector3D m_LightingOrigin;
	int m_DiffuseModulation;
	char gap_38[4];
	int m_collisionFlagsRemove;
};

#endif // GAMEBSPFILE_H