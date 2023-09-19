#ifndef SHADER_VCS_VERSION_H
#define SHADER_VCS_VERSION_H

struct ShaderHeader_t
{
	char* m_pszDebugName;
	uint8_t m_byte1;
	uint8_t m_byte2;
	uint8_t m_byte3;
	uint8_t m_byte4;
	uint8_t m_byte5;
	uint8_t m_byte6;
	uint8_t m_byte7;
	uint8_t m_byte8;
	ID3D11DeviceChild** m_ppShader;
	__int64* m_pUnk2;
};

#endif // SHADER_VCS_VERSION_H