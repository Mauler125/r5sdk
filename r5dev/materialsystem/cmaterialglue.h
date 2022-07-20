#pragma once

#include "materialsystem/cshaderglue.h"

#pragma pack(push, 1) // Without this MSVC might get the idea to align our members to completely fuck the offsets up.
// [ PIXIE ]: The texture GUID's aren't in a specific order, gonna leave them as ptr's so an individual can check them in any memory searcher.
// [ PIXIE ]: Verification needed for earlier seasons, if the struct is the same.
class CMaterialGlue // [ PIXIE ]: Class seems mostly right, a few members are still missing though.
{
public:
	void* m_pVTable; //0x0000
	uint8_t pad_0008[8]; //0x0008
	uint64_t m_GUID; //0x0010
	const char* m_pszName; //0x0018
	const char* m_pszSurfaceName1; //0x0020
	const char* m_pszSurfaceName2; //0x0028
	CMaterialGlue* m_pDepthShadow; //0x0030
	CMaterialGlue* m_pDepthPrepass; //0x0038
	CMaterialGlue* m_pDepthVSM; //0x0040
	CMaterialGlue* m_pDepthShadowTight; //0x0048
	CMaterialGlue* m_pColPass; //0x0050
	CShaderGlue* m_pShaderGlue; //0x0058
	void* m_pTextureGUID; //0x0060
	void* m_pStreamableTextures; //0x0068
	int16_t m_nStreamableTextureCount; //0x0070
	int16_t m_iWidth; //0x0072 
	int16_t m_iHeight; //0x0074
	int16_t m_unused1; //0x0076
	uint32_t m_iFlags; //0x0078 [ PIXIE ]: I'm pretty sure those are VTF Image Flags, If you set them to NULL they cause Texture stretching.
	int32_t m_unused2; //0x007C
	uint8_t pad_0080[8]; //0x0080
	uint32_t m_iUnknownFlags1; //0x0088
	char pad_008C[103]; //0x008C
	uint8_t m_iUnknown1; //0x00F3
	char pad_00F4[12]; //0x00F4
	void* m_pDXBuffer; //0x0100 [ PIXIE ]: ID3D11Buffer*, might need to include dx here.
	void* m_pDXBufferVTable; //0x0108 [ PIXIE ]: ID3D11BufferVtbl, probably just leave it as a void*
	void* m_pUnknown2; //0x0110
	uint32_t m_iUnknown3; //0x0118
	uint16_t m_iUnknown4; //0x011C
	uint16_t m_iUnknown5; //0x011E
	uint16_t m_iUnknown6; //0x0120
	uint64_t m_Unknown7; //0x0122
	uint32_t m_iUnknown8; //0x012A
	uint16_t m_iUnknown9; //0x012E
}; //Size: 0x0130 confirmed end size.
static_assert(sizeof(CMaterialGlue) == 0x130);
#pragma pack(pop)

inline void* g_pMaterialGlueVTable = nullptr;

/* ==== CMATERIALGLUE ================================================================================================================================================== */
inline CMemory p_GetMaterialAtCrossHair;
inline auto GetMaterialAtCrossHair = p_GetMaterialAtCrossHair.RCast<CMaterialGlue* (*)(void)>();

void CMaterialGlue_Attach();
void CMaterialGlue_Detach();
///////////////////////////////////////////////////////////////////////////////
class VMaterialGlue : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CMaterialGlue::GetMaterialAtCrossHair: {:#18x} |\n", p_GetMaterialAtCrossHair.GetPtr());
		spdlog::debug("| CON: g_pMaterialGlueVTable                : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pMaterialGlueVTable));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_GetMaterialAtCrossHair = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x83\xEC\x58\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxxxxxxxx?????");
		GetMaterialAtCrossHair = p_GetMaterialAtCrossHair.RCast<CMaterialGlue* (*)(void)>(); /*48 8B C4 48 83 EC 58 48 83 3D ? ? ? ? ?*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		g_pMaterialGlueVTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xB9\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00"), "x????xxx????xxx????")
			.FindPatternSelf("48 8D ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>(); /*B9 ? ? ? ? 48 8D 05 ? ? ? ? 48 8D 15 ? ? ? ?*/
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VMaterialGlue);