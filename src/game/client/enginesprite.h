//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#ifndef ENGINESPRITE_H
#define ENGINESPRITE_H
#include "public/avi/iavi.h"
#include "public/avi/ibik.h"
#include "public/const.h"
#include "public/imaterial.h"
#include "game/client/hud.h"

//-----------------------------------------------------------------------------
// Purpose: Sprite Models
//-----------------------------------------------------------------------------
class CEngineSprite // !! UNCONFIRMED !!
{
	// NOTE: don't define a constructor or destructor so that this can be allocated
	// as before.
public:
	int GetWidth(void) const { return m_width; }
	int GetHeight(void) const { return m_height; }
	int GetNumFrames(void) const { return m_numFrames; }
	//IMaterial* GetMaterial(RenderMode_t nRenderMode) { return m_material[nRenderMode]; }
	//IMaterial* GetMaterial(RenderMode_t nRenderMode, int nFrame);
	//void SetFrame(RenderMode_t nRenderMode, int nFrame);
	//bool Init(const char* name);
	//void Shutdown(void);
	//void UnloadMaterial();
	//void SetColor(float r, float g, float b);
	//int GetOrientation(void);
	//void GetHUDSpriteColor(float* color);
	float GetUp(void) const { return up; }
	float GetDown(void) const { return down; }
	float GetLeft(void) const { return left; }
	float GetRight(void) const { return right; }
	//void DrawFrame(RenderMode_t nRenderMode, int frame, int x, int y, const wrect_t* prcSubRect);
	//void DrawFrameOfSize(RenderMode_t nRenderMode, int frame, int x, int y, int iWidth, int iHeight, const wrect_t* prcSubRect);
	bool IsAVI(void) const;
	bool IsBIK(void) const;
	//void GetTexCoordRange(float* pMinU, float* pMinV, float* pMaxU, float* pMaxV);

private:
	AVIMaterial_t m_hAVIMaterial;
	BIKMaterial_t m_hBIKMaterial;
	int m_width;
	int m_height;
	int m_numFrames;
	IMaterial* m_material[kRenderModeCount];
	int m_orientation;
	float m_hudSpriteColor[3];
	float up, down, left, right;
};

#endif // ENGINESPRITE_H