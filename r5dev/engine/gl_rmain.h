#pragma once
#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"

#ifndef DEDICATED

// Remove after CViewSetup impl
// For main view calc width / 2 = posX same for posY with height. 
struct TransformInfo_t
{
	int posX;
	int posY;
	int width;
	int height;
};

bool ClipTransform(const VMatrix& w2sMatrix, const Vector3D& point, Vector3D* pClip);
bool ScreenTransform(const TransformInfo_t& transformInfo, const VMatrix& w2sMatrix, const Vector3D& point, Vector3D* pClip);

///////////////////////////////////////////////////////////////////////////////
class VGL_RMain : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif