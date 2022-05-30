#pragma once
#include "public/include/binstream.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int RVPK_DIR_MAGIC = 'Uª4';
constexpr unsigned int RVPK_MAJOR_VER = 2;
constexpr unsigned int RVPK_MINOR_VER = 3;

constexpr unsigned int LIBRARY_PACKS = 2;
constexpr unsigned int LANGUAGE_PACKS = 11;
constexpr unsigned int RVPK_DICT_SIZE = 20;
constexpr int RVPK_MAX_BLOCK = 1024 * 1024;
constexpr int DECOMP_MAX_BUF = 2024 * 2024;

const std::string DIR_LIBRARY_PREFIX[LIBRARY_PACKS] = { "server", "client" };
const std::string DIR_LOCALE_PREFIX[LANGUAGE_PACKS] = { "english", "french", "german", "italian", "japanese", "korean", "polish", "portuguese", "russian", "spanish", "tchinese" };

#pragma pack(push, 1)
struct VPKFileEntry_t
{
	char* directory;
	char* filename;
	char* extension;
	uint8_t unknown[0x38];
};

struct VPKData_t
{
	int m_nHandle;
	char pad[1];
	char m_szPath[255];
	uint8_t unknown2[0x134];
	int32_t m_nEntries;
	uint8_t unknown3[12];
	VPKFileEntry_t* m_pEntries;
};
#pragma pack(pop)

struct VPKEntryDescriptor_t
{
	uint32_t m_nEntryFlags      {}; // Entry flags.
	uint16_t m_nTextureFlags    {}; // Texture flags (only used if the entry is a vtf).
	uint64_t m_nArchiveOffset   {}; // Offset in archive.
	uint64_t m_nCompressedSize  {}; // Compressed size of entry.
	uint64_t m_nUncompressedSize{}; // Uncompressed size of entry.
	bool     m_bIsCompressed  = false;

	VPKEntryDescriptor_t(){};
	VPKEntryDescriptor_t(CIOStream* reader);
	VPKEntryDescriptor_t(uint32_t nEntryFlags, uint16_t nTextureFlags, uint64_t nArchiveOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize);
};

struct VPKEntryBlock_t
{
	uint32_t                     m_nCrc32       {}; // Crc32 for the uncompressed block.
	uint16_t                     m_nPreloadBytes{}; // Preload bytes.
	uint16_t                     m_iArchiveIndex{}; // Index of the archive that contains this block.
	vector<VPKEntryDescriptor_t> m_vvEntries    {}; // Vector of all the entries of a given block (entries have a size limit of 1 MiB, so anything over is split into separate entries within the same block).
	string                       m_svBlockPath  {}; // Path to block within vpk.

	VPKEntryBlock_t(CIOStream* reader, string path);
	VPKEntryBlock_t(const vector<uint8_t>& vData, int64_t nOffset, uint16_t nArchiveIndex, uint32_t nEntryFlags, uint16_t nTextureFlags, string svBlockPath);
};

struct VPKHeader_t
{
	uint32_t                     m_nFileMagic   {}; // File magic.
	uint16_t                     m_nMajorVersion{}; // Vpk major version.
	uint16_t                     m_nMinorVersion{}; // Vpk minor version.
	uint32_t                     m_nTreeSize    {}; // Directory tree size.
	uint32_t                     m_nPadding     {}; // Padding to align the header.
};

struct VPKDir_t
{
	VPKHeader_t                  m_vHeader      {}; // Dir header.
	uint32_t                     m_nFileDataSize{}; // File data section size.
	vector<VPKEntryBlock_t>      m_vvEntryBlocks{}; // Vector of entry blocks.
	uint16_t                     m_iArchiveCount{}; // Highest archive index (archive count-1).
	vector<string>               m_vsvArchives  {}; // Vector of archive file names.
	string                       m_svDirPath    {}; // Path to vpk_dir file.

	VPKDir_t() { m_vHeader.m_nFileMagic = RVPK_DIR_MAGIC; m_vHeader.m_nMajorVersion = RVPK_MAJOR_VER; m_vHeader.m_nMinorVersion = RVPK_MINOR_VER; };
	VPKDir_t(const string& svPath);

	void Build(const string& svFileName, const vector<VPKEntryBlock_t>& vEntryBlocks);
};

class CPackedStore
{
	vector<uint8_t>              m_vHashBuffer      {}; // Buffer for post decomp file validation.
	size_t                       m_nEntryCount      {}; // Entry per-block incrementor.
	lzham_uint32                 m_nAdler32_Internal{}; // Internal operation Adler32 file checksum.
	lzham_uint32                 m_nAdler32         {}; // Pre/post operation Adler32 file checksum.
	lzham_uint32                 m_nCrc32_Internal  {}; // Internal operation Crc32 file checksum.
	lzham_uint32                 m_nCrc32           {}; // Pre/post operation Crc32 file checksum.
	lzham_compress_params        m_lzCompParams     {}; // LZham decompression parameters.
	lzham_compress_status_t      m_lzCompStatus     {}; // LZham compression status.
	lzham_decompress_params      m_lzDecompParams   {}; // LZham decompression parameters.
	lzham_decompress_status_t    m_lzDecompStatus   {}; // LZham decompression status.

public:
	void InitLzCompParams(void);
	void InitLzDecompParams(void);
	VPKDir_t GetPackDirFile(string svPackDirFile);
	string GetPackChunkFile(string svPackDirFile, int iArchiveIndex);
	vector<VPKEntryBlock_t> GetEntryBlocks(CIOStream* reader);
	string FormatBlockPath(string svName, string svPath, string svExtension);
	string StripLocalePrefix(string svPackDirFile);
	void PackAll(string svDirIn, string svPathOut = "");
	void UnpackAll(VPKDir_t vpk, string svPathOut = "");
	void ValidateAdler32PostDecomp(string svDirAsset);
	void ValidateCRC32PostDecomp(string svDirAsset);
};
///////////////////////////////////////////////////////////////////////////////
extern CPackedStore* g_pPackedStore;
