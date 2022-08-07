#pragma once
#include "public/include/binstream.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int VPK_HEADER_MARKER = 0x55AA1234;
constexpr unsigned int VPK_MAJOR_VERSION = 2;
constexpr unsigned int VPK_MINOR_VERSION = 3;
constexpr unsigned int VPK_DICT_SIZE = 20;
constexpr int ENTRY_MAX_LEN = 1024 * 1024;

const vector<string> DIR_CONTEXT = { "server", "client" };
const vector<string> DIR_LOCALE  = { "english", "french", "german", "italian", "japanese", "korean", "polish", "portuguese", "russian", "spanish", "tchinese" };

enum class EPackedLoadFlags : int
{
	LOAD_NONE,
	LOAD_VISIBLE      = 1 << 0,  // FileSystem visibility?
	LOAD_CACHE        = 1 << 8,  // Only set for assets not stored in the depot directory.
	LOAD_TEXTURE_UNK0 = 1 << 18,
	LOAD_TEXTURE_UNK1 = 1 << 19,
	LOAD_TEXTURE_UNK2 = 1 << 20,
};

enum class EPackedTextureFlags : short
{
	TEXTURE_NONE,
	TEXTURE_DEFAULT         = 1 << 3,
	TEXTURE_ENVIRONMENT_MAP = 1 << 10,
};

struct FileHandleTracker_t
{
	int m_nFileNumber;
	int m_nCurOfs;
	HANDLE m_hFileHandle;
};

struct pFileHandleTracker_t
{
	FileHandleTracker_t self[1024];
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

struct VPKChunkDescriptor_t
{
	uint32_t m_nLoadFlags       {}; // Load flags.
	uint16_t m_nTextureFlags    {}; // Texture flags (only used if the entry is a vtf).
	uint64_t m_nArchiveOffset   {}; // Offset in archive.
	uint64_t m_nCompressedSize  {}; // Compressed size of chunk.
	uint64_t m_nUncompressedSize{}; // Uncompressed size of chunk.
	bool     m_bIsCompressed  = false;

	VPKChunkDescriptor_t(){};
	VPKChunkDescriptor_t(CIOStream* pReader);
	VPKChunkDescriptor_t(uint32_t nEntryFlags, uint16_t nTextureFlags, uint64_t nArchiveOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize);
};

struct VPKEntryBlock_t
{
	uint32_t                     m_nFileCRC      {}; // Crc32 for the uncompressed entry.
	uint16_t                     m_iPreloadSize  {}; // Preload bytes.
	uint16_t                     m_iPackFileIndex{}; // Index of the pack file that contains this entry.
	vector<VPKChunkDescriptor_t> m_vChunks       {}; // Vector of all the chunks of a given entry (chunks have a size limit of 1 MiB, anything over this limit is fragmented into smaller chunks).
	string                       m_svEntryPath   {}; // Path to entry within vpk.

	VPKEntryBlock_t(CIOStream* pReader, string svEntryPath);
	VPKEntryBlock_t(const vector<uint8_t>& vData, int64_t nOffset, uint16_t nPreloadData, uint16_t nArchiveIndex, uint32_t nEntryFlags, uint16_t nTextureFlags, const string& svEntryPath);
};

struct VPKDirHeader_t
{
	uint32_t                     m_nHeaderMarker {}; // File magic.
	uint16_t                     m_nMajorVersion {}; // Vpk major version.
	uint16_t                     m_nMinorVersion {}; // Vpk minor version.
	uint32_t                     m_nDirectorySize{}; // Directory tree size.
	uint32_t                     m_nSignatureSize{}; // Directory signature.
};

struct VPKDir_t
{
	VPKDirHeader_t               m_vHeader       {}; // Dir header.
	uint32_t                     m_nFileDataSize {}; // File data section size.
	vector<VPKEntryBlock_t>      m_vEntryBlocks  {}; // Vector of entry blocks.
	uint16_t                     m_iPackFileCount{}; // Highest archive index (archive count-1).
	vector<string>               m_vPackFile     {}; // Vector of archive file names.
	string                       m_svDirPath     {}; // Path to vpk_dir file.

	VPKDir_t(const string& svPath);
	VPKDir_t() { m_vHeader.m_nHeaderMarker = VPK_HEADER_MARKER; m_vHeader.m_nMajorVersion = VPK_MAJOR_VERSION; m_vHeader.m_nMinorVersion = VPK_MINOR_VERSION; };

	void Build(const string& svDirectoryFile, const vector<VPKEntryBlock_t>& vEntryBlocks);
};

struct VPKPair_t
{
	string m_svBlockName;
	string m_svDirectoryName;
};

class CPackedStore
{
public:
	void InitLzCompParams(void);
	void InitLzDecompParams(void);

	VPKDir_t GetDirectoryFile(string svDirectoryFile) const;
	string GetPackFile(const string& svPackDirFile, uint16_t iArchiveIndex) const;

	vector<VPKEntryBlock_t> GetEntryBlocks(CIOStream* pReader) const;
	vector<string> GetEntryPaths(const string& svPathIn) const;
	vector<string> GetEntryPaths(const string& svPathIn, const nlohmann::json& jManifest) const;

	string GetNameParts(const string& svDirectoryName, int nCaptureGroup) const;
	string GetSourceName(const string& svDirectoryName) const;

	nlohmann::json GetManifest(const string& svWorkSpace, const string& svManifestName) const;
	vector<string> GetIgnoreList(const string& svWorkSpace) const;

	string FormatEntryPath(string svName, const string& svPath, const string& svExtension) const;
	string StripLocalePrefix(const string& svDirectoryFile) const;

	VPKPair_t BuildFileName(string svLanguage, string svContext, const string& svPakName, int nPatch) const;
	void BuildManifest(const vector<VPKEntryBlock_t>& vBlock, const string& svWorkSpace, const string& svManifestName) const;

	void PackAll(const VPKPair_t& vPair, const string& svPathIn, const string& svPathOut, bool bManifestOnly);
	void UnpackAll(const VPKDir_t& vDir, const string& svPathOut = "");

	void ValidateAdler32PostDecomp(const string& svDirAsset);
	void ValidateCRC32PostDecomp(const string& svDirAsset);

private:
	size_t                       m_nChunkCount      {}; // Entry per-block incrementor.
	lzham_uint32                 m_nAdler32_Internal{}; // Internal operation Adler32 file checksum.
	lzham_uint32                 m_nAdler32         {}; // Pre/post operation Adler32 file checksum.
	lzham_uint32                 m_nCrc32_Internal  {}; // Internal operation Crc32 file checksum.
	lzham_uint32                 m_nCrc32           {}; // Pre/post operation Crc32 file checksum.
	lzham_compress_params        m_lzCompParams     {}; // LZham decompression parameters.
	lzham_compress_status_t      m_lzCompStatus     {}; // LZham compression status.
	lzham_decompress_params      m_lzDecompParams   {}; // LZham decompression parameters.
	lzham_decompress_status_t    m_lzDecompStatus   {}; // LZham decompression status.
	std::unordered_map<string, VPKChunkDescriptor_t> m_mChunkHashMap{};
};
///////////////////////////////////////////////////////////////////////////////
extern CPackedStore* g_pPackedStore;
