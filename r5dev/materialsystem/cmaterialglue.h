#pragma once

class TXTGuidMap // [ PIXIE ]: Couldn't figure out the other GUID's yet!
{
public:
	void* m_pAlbedoTextureGUID; //0x0000
	void* m_pNormalTextureGUID; //0x0008
	void* m_pSpecTextureGUID; //0x0010
	void* m_pLayerBlendTextureGUID; //0x0018
	void* m_pLightMapTextureRealTimeLightIDGUID; //0x0020
	void* m_pUnknownGUID1; //0x0028
	void* m_pUnknownGUID2; //0x0030
	void* m_pUnknownGUID3; //0x0038
	void* m_pUnknownGUID4; //0x0040
}; //Size: 0x0048

class CMaterialGlue // [ PIXIE ]: Class seems mostly right, a few members are still missing though.
{
public:
	void* m_pVTable; //0x0000
	char pad_0008[8]; //0x0008
	uint64_t m_GUID; //0x0010
	char* m_pszName; //0x0018
	char* m_pszSurfaceName1; //0x0020
	char* m_pszSurfaceName2; //0x0028
	void* m_pDepthShadow; //0x0030
	void* m_pDepthPrepass; //0x0038
	void* m_pDepthVSM; //0x0040
	void* m_pDepthShadowTight; //0x0048
	void* m_pColPass; //0x0050
	void* m_pShaderGlue; //0x0058
	TXTGuidMap* m_pTextureGUID; //0x0060
	TXTGuidMap* m_pTextureGUID2; //0x0068
	char pad_0070[4]; //0x0070
	int32_t m_iTextureRes; //0x0074
	char pad_0078[184]; //0x0078
}; //Size: 0x0130

namespace
{
	/* ==== CMATERIALGLUE ================================================================================================================================================== */

}

void CMaterialGlue_Attach();
void CMaterialGlue_Detach();
///////////////////////////////////////////////////////////////////////////////
class HCMaterialGlue : public IDetour
{
	virtual void debugp()
	{

	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCMaterialGlue);