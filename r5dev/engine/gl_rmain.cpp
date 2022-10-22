//=====================================================================================//
//
// Purpose:
//
// $NoKeywords: $
//=====================================================================================//
#include "core/stdafx.h"

#ifndef DEDICATED

#include "engine/gl_rmain.h"
#include "materialsystem/cmaterialsystem.h"

//-----------------------------------------------------------------------------
// Purpose: compute the scene coordinates of a point in 3D
// Input  : &w2sMatrix -
//          &point -
//          *pClip -
// Output : is offscreen?
//-----------------------------------------------------------------------------
bool ClipTransform(const VMatrix& w2sMatrix, const Vector3D& point, Vector3D* pClip)
{
	pClip->x = w2sMatrix[0][0] * point.x + w2sMatrix[0][1] * point.y + w2sMatrix[0][2] * point.z + w2sMatrix[0][3];
	pClip->y = w2sMatrix[1][0] * point.x + w2sMatrix[1][1] * point.y + w2sMatrix[1][2] * point.z + w2sMatrix[1][3];
	pClip->z = 0.0f;

	float w = w2sMatrix[3][0] * point.x + w2sMatrix[3][1] * point.y + w2sMatrix[3][2] * point.z + w2sMatrix[3][3];

	if (w < 0.001f)
	{
		// Clamp here.
		pClip->x *= 100000;
		pClip->y *= 100000;
		return true;
	}

	float invw = 1.0f / w;
	pClip->x *= invw;
	pClip->y *= invw;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: translate point to screen position
// Input  : &transformInfo -
//          &w2sMatrix -
//          &point -
//          *pClip -
// Output : is offscreen?
//-----------------------------------------------------------------------------
bool ScreenTransform(const TransformInfo_t& transformInfo, const VMatrix& w2sMatrix, const Vector3D& point, Vector3D* pClip)
{
	bool bIsOffscreen = ClipTransform(w2sMatrix, point, pClip);

	// is offscreen?
	if (!bIsOffscreen)
	{
		pClip->x = (transformInfo.width * 0.5f) + (pClip->x * transformInfo.width) * 0.5f + transformInfo.posX;
		pClip->y = (transformInfo.height * 0.5f) - (pClip->y * transformInfo.height) * 0.5f + transformInfo.posY;

		return true;
	}

	return false;
}

#endif