#pragma once

#pragma pack(push, 1) // Without this MSVC might get the idea to align our members to completely fuck the offsets up.
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
	void* m_pShaderGlue; //0x0058 [ PIXIE ] TODO: Reverse CShaderGlue.
	void* m_pTextureGUID1; //0x0060
	void* m_pTextureGUID2; //0x0068
	std::int16_t m_UnknownSignature; //0x0070 [ PIXIE ]: This seems to be the start of a modified VTF Header, I have no clue what this member does. 
	std::int16_t m_iWidth; //0x0072 
	std::int16_t m_iHeight; //0x0074
	std::int16_t m_unused1; //0x0076
	std::uint32_t m_iFlags; //0x0078 [ PIXIE ]: I'm pretty sure those are VTF Image Flags, If you set them to NULL they cause Texture stretching.
	std::int32_t m_unused2; //0x007C
	char pad_0080[8]; //0x0080
	std::uint32_t m_iUnknownFlags1; //0x0088
	char pad_008C[116]; //0x008C

	// They first point to a jump table which holds the texture, then theres another jump onto the actual texture.
	void** m_ppDXTexture1; //0x0100
	void** m_ppDXTexture2; //0x0108
	char pad_0110[8]; //0x0110
	std::uint32_t m_iUnknown1; //0x0118
	std::uint16_t m_iUnknown2; //0x011C
	std::uint16_t m_iUnknown3; //0x011E
	std::uint16_t m_iUnknown4; //0x0120
	std::uint64_t m_Unknown5; //0x0122
	std::uint32_t m_iUnknown6; //0x012A
	std::uint16_t m_iUnknown7; //0x012E
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