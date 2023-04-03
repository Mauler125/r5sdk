#ifndef PACKEDSTORE_H
#define PACKEDSTORE_H
/*******************************************************************
* ██████╗  ██╗    ██╗   ██╗██████╗ ██╗  ██╗    ██╗     ██╗██████╗  *
* ██╔══██╗███║    ██║   ██║██╔══██╗██║ ██╔╝    ██║     ██║██╔══██╗ *
* ██████╔╝╚██║    ██║   ██║██████╔╝█████╔╝     ██║     ██║██████╔╝ *
* ██╔══██╗ ██║    ╚██╗ ██╔╝██╔═══╝ ██╔═██╗     ██║     ██║██╔══██╗ *
* ██║  ██║ ██║     ╚████╔╝ ██║     ██║  ██╗    ███████╗██║██████╔╝ *
* ╚═╝  ╚═╝ ╚═╝      ╚═══╝  ╚═╝     ╚═╝  ╚═╝    ╚══════╝╚═╝╚═════╝  *
*******************************************************************/
#include "public/ifilesystem.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int VPK_HEADER_MARKER = 0x55AA1234;
constexpr unsigned int VPK_MAJOR_VERSION = 2;
constexpr unsigned int VPK_MINOR_VERSION = 3;
constexpr unsigned int VPK_DICT_SIZE = 20;
constexpr int ENTRY_MAX_LEN = 1024 * 1024;
constexpr int PACKFILEPATCH_MAX = 512;
constexpr int PACKFILEINDEX_SEP = 0x0;
constexpr int PACKFILEINDEX_END = 0xffff;

static const std::regex BLOCK_REGEX{ R"(pak000_([0-9]{3}))" };
static const std::regex DIR_REGEX{ R"((?:.*\/)?([^_]*_)(.*)(.bsp.pak000_dir).*)" };

static const vector<const char*> DIR_TARGET = 
{
	"server",
	"client"
};
static const vector<const char*> DIR_LOCALE  =
{
	"english",
	"french",
	"german",
	"italian",
	"japanese",
	"korean",
	"polish",
	"portuguese",
	"russian",
	"spanish",
	"tchinese"
};

struct VPKKeyValues_t
{
	static constexpr uint16_t TEXTURE_FLAGS_DEFAULT = static_cast<uint16_t>(EPackedTextureFlags::TEXTURE_DEFAULT);
	static constexpr uint32_t LOAD_FLAGS_DEFAULT = static_cast<uint32_t>(EPackedLoadFlags::LOAD_VISIBLE) | static_cast<uint32_t>(EPackedLoadFlags::LOAD_CACHE);

	string m_svEntryPath;
	uint16_t m_iPreloadSize;
	uint32_t m_nLoadFlags;
	uint16_t m_nTextureFlags;
	bool m_bUseCompression;
	bool m_bDeduplicate;

	VPKKeyValues_t(const string& svEntryPath = "", uint16_t iPreloadSize = NULL, uint32_t nLoadFlags = LOAD_FLAGS_DEFAULT, 
		uint16_t nTextureFlags = TEXTURE_FLAGS_DEFAULT, bool bUseCompression = true, bool bDeduplicate = true);
};

struct VPKChunkDescriptor_t
{
	uint32_t m_nLoadFlags;        // Load flags.
	uint16_t m_nTextureFlags;     // Texture flags (only used if the entry is a vtf).
	uint64_t m_nPackFileOffset;   // Offset in pack file.
	uint64_t m_nCompressedSize;   // Compressed size of chunk.
	uint64_t m_nUncompressedSize; // Uncompressed size of chunk.

	VPKChunkDescriptor_t(FileHandle_t hDirectoryFile);
	VPKChunkDescriptor_t(uint32_t nLoadFlags, uint16_t nTextureFlags, uint64_t nPackFileOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize);
};

struct VPKEntryBlock_t
{
	uint32_t                     m_nFileCRC;       // Crc32 for the uncompressed entry.
	uint16_t                     m_iPreloadSize;   // Preload bytes.
	uint16_t                     m_iPackFileIndex; // Index of the pack file that contains this entry.
	vector<VPKChunkDescriptor_t> m_vFragments;     // Vector of all the chunks of a given entry (chunks have a size limit of 1 MiB, anything over this limit is fragmented into smaller chunks).
	string                       m_svEntryPath;    // Path to entry within vpk.

	VPKEntryBlock_t(FileHandle_t pFile, const string& svEntryPath);
	VPKEntryBlock_t(const uint8_t* pData, size_t nLen, int64_t nOffset, uint16_t iPreloadSize,
		uint16_t iPackFileIndex, uint32_t nLoadFlags, uint16_t nTextureFlags, const string& svEntryPath);
};

struct VPKDirHeader_t
{
	uint32_t                     m_nHeaderMarker;  // File magic.
	uint16_t                     m_nMajorVersion;  // Vpk major version.
	uint16_t                     m_nMinorVersion;  // Vpk minor version.
	uint32_t                     m_nDirectorySize; // Directory tree size.
	uint32_t                     m_nSignatureSize; // Directory signature.
};

struct VPKPair_t
{
	string m_svPackName;
	string m_svDirectoryName;

	VPKPair_t(const string& svLocale, const string& svTarget, const string& svLevel, int nPatch);
};

struct VPKDir_t
{
	VPKDirHeader_t               m_vHeader;        // Dir header.
	vector<VPKEntryBlock_t>      m_vEntryBlocks;   // Vector of entry blocks.
	uint16_t                     m_nPackFileCount; // Number of pack patches (pack file count-1).
	vector<string>               m_vPackFile;      // Vector of pack file names.
	string                       m_svDirectoryPath;// Path to vpk_dir file.

	VPKDir_t()
	{
		m_vHeader.m_nHeaderMarker = VPK_HEADER_MARKER; m_vHeader.m_nMajorVersion = VPK_MAJOR_VERSION; 
		m_vHeader.m_nMinorVersion = VPK_MINOR_VERSION; m_vHeader.m_nDirectorySize = NULL, m_vHeader.m_nSignatureSize = NULL;
		m_nPackFileCount = NULL;
	};
	VPKDir_t(const string& svDirectoryFile);
	VPKDir_t(const string& svDirectoryFile, bool bSanitizeName);

	void Init(const string& svPath);

	string StripLocalePrefix(const string& svDirectoryFile) const;
	string GetPackFile(const string& svDirectoryPath, uint16_t iPackFileIndex) const;

	void WriteHeader(FileHandle_t hDirectoryFile) const;
	void WriteTreeSize(FileHandle_t hDirectoryFile) const;
	uint64_t WriteDescriptor(FileHandle_t hDirectoryFile, std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>& vMap) const;

	void BuildDirectoryTree(const vector<VPKEntryBlock_t>& vEntryBlocks, std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>& vMap) const;
	void BuildDirectoryFile(const string& svDirectoryFile, const vector<VPKEntryBlock_t>& vEntryBlocks);
};

class CPackedStore
{
public:
	void InitLzCompParams(void);
	void InitLzDecompParams(void);

	lzham_compress_level GetCompressionLevel(void) const;

	vector<VPKEntryBlock_t> GetEntryBlocks(FileHandle_t hDirectoryFile) const;
	vector<VPKKeyValues_t> GetEntryValues(const string& svWorkspace) const;
	vector<VPKKeyValues_t> GetEntryValues(const string& svWorkspace, KeyValues* pManifestKeyValues) const;

	string GetNameParts(const string& svDirectoryName, int nCaptureGroup) const;
	string GetLevelName(const string& svDirectoryName) const;

	KeyValues* GetManifest(const string& svWorkspace, const string& svManifestName) const;
	vector<string> GetIgnoreList(const string& svWorkspace) const;

	string FormatEntryPath(const string& svPath, const string& svName, const string& svExtension) const;
	void BuildManifest(const vector<VPKEntryBlock_t>& vBlock, const string& svWorkspace, const string& svManifestName) const;

	void ValidateCRC32PostDecomp(const string& svAssetPath, const uint32_t nFileCRC);
	bool Deduplicate(const uint8_t* pEntryBuffer, VPKChunkDescriptor_t& descriptor, const size_t chunkIndex);

	void PackWorkspace(const VPKPair_t& vPair, const string& svWorkspace, const string& svBuildPath, bool bManifestOnly);
	void UnpackWorkspace(const VPKDir_t& vDirectory, const string& svWorkspace = "");

private:
	size_t                       m_nChunkCount;       // The number of fragments for this asset.
	lzham_compress_params        m_lzCompParams;      // LZham compression parameters.
	lzham_decompress_params      m_lzDecompParams;    // LZham decompression parameters.
	std::unordered_map<string, const VPKChunkDescriptor_t&> m_mChunkHashMap;
};
///////////////////////////////////////////////////////////////////////////////
extern CPackedStore* g_pPackedStore;

#endif // PACKEDSTORE_H