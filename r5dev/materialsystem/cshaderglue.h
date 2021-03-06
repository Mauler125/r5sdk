#pragma once

#pragma pack(push, 1)
class CShaderGlue // Most of these found in CShaderGlue::SetupShader
{
public:
	int SetupShader(uint64_t nCount, uint64_t a3, void* pRawMaterialGlueWithoutVTable);

	void* m_pVTable; //0x0000
	char pad_0008[8]; //0x0008 Dispatcher Context, Some SEH try and catch thing.
	uint64_t m_nUnknown1; //0x0010
	uint16_t m_nCount1; //0x0018
	uint16_t m_nCount2; //0x001A
	uint16_t m_nNumSamplers; //0x001C [ PIXIE ]: Used by ID3D11DeviceContext::PSSetSamplers to set NumSamplers
	uint8_t m_nStartSlot; //0x001E [ PIXIE ]: Used by ID3D11DeviceContext::PSSetShaderResources to set StartSlot.
	uint8_t m_nNumViews; //0x001F [ PIXIE ]: Used by ID3D11DeviceContext::PSSetShaderResources to set NumViews.
	uint8_t m_nByte1; //0x0020 [ PIXIE ]: No clue tbh the only usage is an example I will show below. All of the usages are cmp also..?
	uint8_t pad_0021[15]; //0x0021
	void** m_ppVertexShader; //0x0030 [ PIXIE ]: Points to another structure which holds a double ptr to d3d11.dll
	void** m_ppPixelShader; //0x0038 [ PIXIE ]: Points to another structure which holds a double ptr to d3d11.dll
}; //Size: 0x0040
static_assert(sizeof(CShaderGlue) == 0x40); // [ PIXIE ]: All vars have proper datatype size, paddings are unknown.
#pragma pack(pop)

/*
  if ( *(_BYTE *)(v19 + a1 + 32) != byte_14171A08A ) // (v19 + a1 + 32) = m_nByte1
  {
	byte_14171A08A = *(_BYTE *)(v19 + a1 + 32);
	qword_14171AE78 = -1i64;
  }
*/

/* ==== CSHADERGLUE ================================================================================================================================================== */
inline auto CShaderGlue_SetupShader = CMemory().RCast<int(*)(CShaderGlue* thisptr, uint64_t nCount, uint64_t a3, void* pRawMaterialGlueWithoutVTable)>();

inline CMemory CShaderGlue_VTable;
inline void* g_pCShaderGlue_VTable = nullptr;

void CShaderGlue_Attach();
void CShaderGlue_Detach();
///////////////////////////////////////////////////////////////////////////////
class VShaderGlue : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const 
	{
		// We get it here in GetFun because we grab other functions with it, it's more efficient.
		CShaderGlue_VTable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x01\xC3\xCC\xCC\xCC\xCC\xCC\x48\x85\xD2"), "xxx????xxxxxxxxxxxx").ResolveRelativeAddressSelf(0x3, 0x7);
		g_pCShaderGlue_VTable = CShaderGlue_VTable.RCast<void*>(); /*48 8D 05 ? ? ? ? 48 89 01 C3 CC CC CC CC CC 48 85 D2*/

		CShaderGlue_SetupShader = CShaderGlue_VTable.WalkVTable(4).RCast<int(*)(CShaderGlue*, uint64_t, uint64_t, void*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VShaderGlue);