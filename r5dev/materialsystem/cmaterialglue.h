#pragma once
#ifndef DEDICATED
#include "materialsystem/cshaderglue.h"
#include "public/imaterialinternal.h"
#include "public/materialsystem/shader_vcs_version.h"
#include "public/rendersystem/schema/texture.g.h"

struct CMaterialGlue_Unknown
{
	__m128i unk1;
	__m128i unk2;
	__m128i unk3;
};

#pragma pack(push, 1) // Without this MSVC might get the idea to align our members to completely fuck the offsets up.
// [ PIXIE ]: The texture GUID's aren't in a specific order, gonna leave them as ptr's so an individual can check them in any memory searcher.
// [ PIXIE ]: Verification needed for earlier seasons, if the struct is the same.
// [ PIXIE ]: Class seems mostly right, a few members are still missing though.
class CMaterialGlue : public IMaterialInternal
{
public:
	uint8_t pad_0008[8]; //0x0008
	uint64_t m_GUID; //0x0010
	const char* m_pszName; //0x0018
	const char* m_pszSurfaceProp; //0x0020
	const char* m_pszSurfaceProp2; //0x0028
	CMaterialGlue* m_pDepthShadow; //0x0030
	CMaterialGlue* m_pDepthPrepass; //0x0038
	CMaterialGlue* m_pDepthVSM; //0x0040
	CMaterialGlue* m_pDepthShadowTight; //0x0048
	CMaterialGlue* m_pColPass; //0x0050
	CShaderGlue* m_pShaderGlue; //0x0058
	TextureHeader_t** m_pTextureHandles; //0x0060
	TextureHeader_t** m_pStreamableTextureHandles; //0x0068
	int16_t m_nStreamableTextureCount; //0x0070
	int16_t m_iWidth; //0x0072 
	int16_t m_iHeight; //0x0074
	int16_t m_iDepth; //0x0076
	uint32_t m_iFlags; //0x0078 [ PIXIE ]: I'm pretty sure those are VTF Image Flags, If you set them to NULL they cause Texture stretching.
	int32_t m_unused2; //0x007C
	uint8_t pad_0080[8]; //0x0080
	uint32_t m_iUnknownFlags1; //0x0088
	char pad_008C[4]; //0x008C
	CMaterialGlue_Unknown m_UnkSections[2];
	uint8_t bytef0;
	uint8_t bytef1;
	uint8_t m_iMaterialType;
	uint8_t bytef3;
	int dwordf4;
	void* m_pTextureAnim;
	void** m_pDXBuffer;
	void** m_pID3D11BufferVTable;
	void* m_pViewsBuffer;
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
#endif // !DEDICATED

inline void* g_pMaterialGlueVFTable = nullptr;


/* ==== CMATERIALGLUE ================================================================================================================================================== */
#ifndef DEDICATED
inline CMemory p_GetMaterialAtCrossHair;
inline CMaterialGlue*(*GetMaterialAtCrossHair)(void);
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VMaterialGlue : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CMaterialGlue::`vftable'", reinterpret_cast<uintptr_t>(g_pMaterialGlueVFTable));
#ifndef DEDICATED
		LogFunAdr("CMaterialGlue::GetMaterialAtCrossHair", p_GetMaterialAtCrossHair.GetPtr());
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
#ifndef DEDICATED
		p_GetMaterialAtCrossHair = g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC 58 48 83 3D ?? ?? ?? ?? ??");
		GetMaterialAtCrossHair = p_GetMaterialAtCrossHair.RCast<CMaterialGlue* (*)(void)>(); /*48 8B C4 48 83 EC 58 48 83 3D ? ? ? ? ?*/
#endif // !DEDICATED
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		g_pMaterialGlueVFTable = g_GameDll.GetVirtualMethodTable(".?AVCMaterialGlue@@");
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
