#pragma once

#pragma pack(push, 1)
class CShaderGlue
{
public:
	void* m_pVTable; //0x0000
	uint16_t m_nCount1; //0x0008
	uint16_t m_nCount2; //0x000A
	uint16_t m_nCount3; //0x000C
	uint8_t m_nByte1; //0x000E
	uint8_t m_nByte2; //0x000F
	char pad_0010[32]; //0x0010 [ PIXIE ]: Unknown, Due to compiler deciding to copy over 16 bytes at once in the replace function I have no clue what the data size is.
	void* m_pVertexShader; //0x0030 [ PIXIE ]: Points to another structure which holds a double ptr to d3d11.dll
	void* m_pPixelShader; //0x0038 [ PIXIE ]: Points to another structure which holds a double ptr to d3d11.dll
}; //Size: 0x0040
static_assert(sizeof(CShaderGlue) == 0x40);
#pragma pack(pop)

/* ==== CSHADERGLUE ================================================================================================================================================== */

void CShaderGlue_Attach();
void CShaderGlue_Detach();
///////////////////////////////////////////////////////////////////////////////
class VShaderGlue : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VShaderGlue);