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
#include "public/tier1/strtools.h"
#include "public/tier1/utlvector.h"
#include "public/tier1/utlstring.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int VPK_HEADER_MARKER = 0x55AA1234;
constexpr unsigned int VPK_MAJOR_VERSION = 2;
constexpr unsigned int VPK_MINOR_VERSION = 3;
constexpr unsigned int VPK_DICT_SIZE = 20;
constexpr int ENTRY_MAX_LEN = 1024 * 1024;
constexpr int PACKFILEPATCH_MAX = 512;
constexpr int PACKFILEINDEX_SEP = 0x0;
constexpr int PACKFILEINDEX_END = 0xffff;
constexpr const char VPK_IGNORE_FILE[] = ".vpkignore";

static const char* const DIR_TARGET[]
{
	"server",
	"client"
};
static const char* const DIR_LOCALE[]
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

	CUtlString m_EntryPath;
	uint16_t m_iPreloadSize;
	uint32_t m_nLoadFlags;
	uint16_t m_nTextureFlags;
	bool m_bUseCompression;
	bool m_bDeduplicate;

	VPKKeyValues_t(const CUtlString& svEntryPath = "", uint16_t iPreloadSize = NULL, uint32_t nLoadFlags = LOAD_FLAGS_DEFAULT,
		uint16_t nTextureFlags = TEXTURE_FLAGS_DEFAULT, bool bUseCompression = true, bool bDeduplicate = true);
};

struct VPKChunkDescriptor_t
{
	uint32_t m_nLoadFlags;        // Load flags.
	uint16_t m_nTextureFlags;     // Texture flags (only used if the entry is a vtf).
	uint64_t m_nPackFileOffset;   // Offset in pack file.
	uint64_t m_nCompressedSize;   // Compressed size of chunk.
	uint64_t m_nUncompressedSize; // Uncompressed size of chunk.

	VPKChunkDescriptor_t()
		: m_nLoadFlags(0)
		, m_nTextureFlags(0)
		, m_nPackFileOffset(0)
		, m_nCompressedSize(0)
		, m_nUncompressedSize(0)
	{
	}
	VPKChunkDescriptor_t(FileHandle_t hDirectoryFile);
	VPKChunkDescriptor_t(uint32_t nLoadFlags, uint16_t nTextureFlags, uint64_t nPackFileOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize);
};

struct VPKEntryBlock_t
{
	uint32_t                         m_nFileCRC;       // Crc32 for the uncompressed entry.
	uint16_t                         m_iPreloadSize;   // Preload bytes.
	uint16_t                         m_iPackFileIndex; // Index of the pack file that contains this entry.
	CUtlVector<VPKChunkDescriptor_t> m_Fragments;      // Vector of all the chunks of a given entry (chunks have a size limit of 1 MiB, anything over this limit is fragmented into smaller chunks).
	CUtlString                       m_EntryPath;      // Path to entry within vpk.

	VPKEntryBlock_t(FileHandle_t pFile, const char* svEntryPath);
	VPKEntryBlock_t(const uint8_t* pData, size_t nLen, int64_t nOffset, uint16_t iPreloadSize,
		uint16_t iPackFileIndex, uint32_t nLoadFlags, uint16_t nTextureFlags, const char* svEntryPath);

	VPKEntryBlock_t(const VPKEntryBlock_t& other)
		: m_nFileCRC(other.m_nFileCRC)
		, m_iPreloadSize(other.m_iPreloadSize)
		, m_iPackFileIndex(other.m_iPackFileIndex)
		, m_EntryPath(other.m_EntryPath)
	{
		// Has to be explicitly copied!
		m_Fragments = other.m_Fragments;
	}
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
	CUtlString m_PackName;
	CUtlString m_DirName;

	VPKPair_t(const char* svLocale, const char* svTarget, const char* svLevel, int nPatch);
};

struct VPKDir_t
{
	VPKDirHeader_t               m_Header;        // Dir header.
	CUtlVector<VPKEntryBlock_t>  m_EntryBlocks;   // Vector of entry blocks.
	uint16_t                     m_PackFileCount; // Number of pack patches (pack file count-1).
	CUtlVector<CUtlString>       m_PackFiles;     // Vector of pack file names.
	CUtlString                   m_DirFilePath;   // Path to vpk_dir file.

	VPKDir_t()
	{
		m_Header.m_nHeaderMarker = VPK_HEADER_MARKER; m_Header.m_nMajorVersion = VPK_MAJOR_VERSION; 
		m_Header.m_nMinorVersion = VPK_MINOR_VERSION; m_Header.m_nDirectorySize = NULL, m_Header.m_nSignatureSize = NULL;
		m_PackFileCount = NULL;
	};
	VPKDir_t(const CUtlString& svDirectoryFile);
	VPKDir_t(const CUtlString& svDirectoryFile, bool bSanitizeName);

	void Init(const CUtlString& svPath);

	CUtlString StripLocalePrefix(const CUtlString& svDirectoryFile) const;
	CUtlString GetPackFile(const CUtlString& svDirectoryPath, uint16_t iPackFileIndex) const;

	void WriteHeader(FileHandle_t hDirectoryFile) const;
	void WriteTreeSize(FileHandle_t hDirectoryFile) const;
	uint64_t WriteDescriptor(FileHandle_t hDirectoryFile, std::map<CUtlString, std::map<CUtlString, std::list<VPKEntryBlock_t>>>& vMap) const;

	void BuildDirectoryTree(const CUtlVector<VPKEntryBlock_t>& entryBlocks, std::map<CUtlString, std::map<CUtlString, std::list<VPKEntryBlock_t>>>& vMap) const;
	void BuildDirectoryFile(const CUtlString& svDirectoryFile, const CUtlVector<VPKEntryBlock_t>& entryBlocks);
};

class CPackedStore
{
public:
	void InitLzCompParams(void);
	void InitLzDecompParams(void);

	lzham_compress_level GetCompressionLevel(void) const;

	void GetEntryBlocks(CUtlVector<VPKEntryBlock_t>& entryBlocks, FileHandle_t hDirectoryFile) const;
	bool GetEntryValues(CUtlVector<VPKKeyValues_t>& entryValues, const CUtlString& workspacePath, const CUtlString& dirFileName) const;

	CUtlString GetNameParts(const CUtlString& dirFileName, int nCaptureGroup) const;
	CUtlString GetLevelName(const CUtlString& dirFileName) const;

	KeyValues* GetManifest(const CUtlString& workspacePath, const CUtlString& manifestFile) const;
	bool GetIgnoreList(CUtlVector<CUtlString>& ignoreList, const CUtlString& workspacePath) const;

	CUtlString FormatEntryPath(const CUtlString& filePath, const CUtlString& fileName, const CUtlString& fileExt) const;
	void BuildManifest(const CUtlVector<VPKEntryBlock_t>& entryBlocks, const CUtlString& workspacePath, const CUtlString& manifestName) const;

	void ValidateCRC32PostDecomp(const CUtlString& assetPath, const uint32_t nFileCRC);
	bool Deduplicate(const uint8_t* pEntryBuffer, VPKChunkDescriptor_t& descriptor, const size_t chunkIndex);

	bool ShouldPrune(const CUtlString& filePath, CUtlVector<CUtlString>& ignoreList) const;

	void PackWorkspace(const VPKPair_t& vpkPair, const char* workspaceName, const char* buildPath);
	void UnpackWorkspace(const VPKDir_t& vpkDir, const char* workspaceName = "");

private:
	lzham_compress_params                                   m_lzCompParams;   // LZham compression parameters.
	lzham_decompress_params                                 m_lzDecompParams; // LZham decompression parameters.
	std::unordered_map<string, const VPKChunkDescriptor_t&> m_ChunkHashMap;
};
///////////////////////////////////////////////////////////////////////////////
extern CPackedStore* g_pPackedStore;

#endif // PACKEDSTORE_H