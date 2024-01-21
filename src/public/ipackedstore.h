#ifndef IPACKEDSTORE_H
#define IPACKEDSTORE_H

enum EPackedLoadFlags
{
	LOAD_NONE,
	LOAD_VISIBLE = 1 << 0,  // Visible to FileSystem.
	LOAD_CACHE = 1 << 8,  // Only set for assets not stored in the depot directory.
	LOAD_TEXTURE_UNK0 = 1 << 18,
	LOAD_TEXTURE_UNK1 = 1 << 19,
	LOAD_TEXTURE_UNK2 = 1 << 20,
};

enum EPackedTextureFlags
{
	TEXTURE_NONE,
	TEXTURE_DEFAULT = 1 << 3,
	TEXTURE_ENVIRONMENT_MAP = 1 << 10,
};

enum EPackedStoreTargets
{
	STORE_TARGET_SERVER,
	STORE_TARGET_CLIENT
};

#pragma pack(push, 1)
struct VPKFileEntry_t
{
	char* m_pszDirectory;
	char* m_pszFileName;
	char* m_pszExtension;
	uint8_t unknown[0x38];
};

struct VPKData_t
{
	int             m_nHandle;
	char            pad[1];
	char            m_szPath[255];
	uint8_t         unknown2[0x134];
	int32_t         m_nEntries;
	uint8_t         unknown3[12];
	VPKFileEntry_t* m_pEntries;
};
#pragma pack(pop)

#endif // IPACKEDSTORE_H