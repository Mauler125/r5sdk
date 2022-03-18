#pragma once

// [ PIXIE ]: The texture GUID's aren't in a specific order, gonna leave them as ptr's so an individual can check them in any memory searcher.
// [ PIXIE ]: Verification needed for earlier seasons, if the struct is the same.
class CMaterialGlue // [ PIXIE ]: Class seems mostly right, a few members are still missing though.
{
public:
	void* m_pVTable; //0x0000
	char pad_0008[8]; //0x0008
	std::uint64_t m_GUID; //0x0010
	char* m_pszName; //0x0018
	char* m_pszSurfaceName1; //0x0020
	char* m_pszSurfaceName2; //0x0028
	CMaterialGlue* m_pDepthShadow; //0x0030
	CMaterialGlue* m_pDepthPrepass; //0x0038
	CMaterialGlue* m_pDepthVSM; //0x0040
	CMaterialGlue* m_pDepthShadowTight; //0x0048
	CMaterialGlue* m_pColPass; //0x0050
	void* m_pShaderGlue; //0x0058 // [ PIXIE ] TODO: Reverse CShaderGlue.
	void* m_pTextureGUID1; //0x0060
	void* m_pTextureGUID2; //0x0068
	char pad_0070[4]; //0x0070
	int32_t m_iMaterialRes; //0x0074
	char pad_0078[184]; //0x0078
}; //Size: 0x0130

namespace
{
	/* ==== CMATERIALGLUE ================================================================================================================================================== */
	ADDRESS p_GetMaterialAtCrossHair = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x83\xEC\x58\x48\x83\x3D\x00\x00\x00\x00\x00", "xxxxxxxxxx?????");
	CMaterialGlue* (*GetMaterialAtCrossHair)(void) = (CMaterialGlue*(*)(void))p_GetMaterialAtCrossHair.GetPtr(); /*48 8B C4 48 83 EC 58 48 83 3D ? ? ? ? ?*/
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