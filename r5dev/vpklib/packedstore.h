/*******************************************************************
* ██████╗  ██╗    ██╗   ██╗██████╗ ██╗  ██╗    ██╗     ██╗██████╗  *
* ██╔══██╗███║    ██║   ██║██╔══██╗██║ ██╔╝    ██║     ██║██╔══██╗ *
* ██████╔╝╚██║    ██║   ██║██████╔╝█████╔╝     ██║     ██║██████╔╝ *
* ██╔══██╗ ██║    ╚██╗ ██╔╝██╔═══╝ ██╔═██╗     ██║     ██║██╔══██╗ *
* ██║  ██║ ██║     ╚████╔╝ ██║     ██║  ██╗    ███████╗██║██████╔╝ *
* ╚═╝  ╚═╝ ╚═╝      ╚═══╝  ╚═╝     ╚═╝  ╚═╝    ╚══════╝╚═╝╚═════╝  *
*******************************************************************/
#ifndef PACKEDSTORE_H
#define PACKEDSTORE_H

#include "public/const.h"
#include "public/ipackedstore.h"
#include "public/ifilesystem.h"
#include "public/tier1/strtools.h"
#include "public/tier1/utlvector.h"
#include "public/tier1/utlstring.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int VPK_HEADER_MARKER = 0x55AA1234;
constexpr unsigned int VPK_MAJOR_VERSION = 2;
constexpr unsigned int VPK_MINOR_VERSION = 3;
constexpr unsigned int VPK_DICT_SIZE = 20;
constexpr unsigned int VPK_ENTRY_MAX_LEN = 1024 * 1024;
constexpr int PACKFILEPATCH_MAX = 512;
constexpr int PACKFILEINDEX_SEP = 0x0;
constexpr int PACKFILEINDEX_END = 0xffff;
constexpr const char VPK_IGNORE_FILE[] = ".vpkignore";

static const std::regex g_VpkDirFileRegex{ R"((?:.*\/)?([^_]*)(?:_)(.*)(.bsp.pak000_dir).*)" };
static const std::regex g_VpkPackFileRegex{ R"(pak000_([0-9]{3}))" };

//-----------------------------------------------------------------------------
// KeyValues structure for the VPK manifest file. This struct gets populated by
// the VPK's corresponding manifest file, which ultimately determines how each
// asset is getting packed into the VPK.
//-----------------------------------------------------------------------------
struct VPKKeyValues_t
{
	static constexpr uint16_t TEXTURE_FLAGS_DEFAULT =
		EPackedTextureFlags::TEXTURE_DEFAULT;

	static constexpr uint32_t LOAD_FLAGS_DEFAULT =
		EPackedLoadFlags::LOAD_VISIBLE | EPackedLoadFlags::LOAD_CACHE;

	CUtlString m_EntryPath;
	uint16_t m_iPreloadSize;
	uint32_t m_nLoadFlags;
	uint16_t m_nTextureFlags;
	bool m_bUseCompression;
	bool m_bDeduplicate;

	VPKKeyValues_t(const CUtlString& svEntryPath = "",
		uint16_t iPreloadSize = NULL,
		uint32_t nLoadFlags = LOAD_FLAGS_DEFAULT,
		uint16_t nTextureFlags = TEXTURE_FLAGS_DEFAULT,
		bool bUseCompression = true, bool bDeduplicate = true);
};

//-----------------------------------------------------------------------------
// An asset packed into a VPK is carved into 'ENTRY_MAX_LEN' chunks, the chunk
// is then optionally compressed. A chunk is NOT compressed if the compressed
// size equals the uncompressed size.
//-----------------------------------------------------------------------------
struct VPKChunkDescriptor_t
{
	uint32_t m_nLoadFlags;

	// Texture flags (only used if the entry is a vtf).
	uint16_t m_nTextureFlags;

	// Offset in pack file.
	uint64_t m_nPackFileOffset;
	uint64_t m_nCompressedSize;
	uint64_t m_nUncompressedSize;

	VPKChunkDescriptor_t()
		: m_nLoadFlags(0)
		, m_nTextureFlags(0)
		, m_nPackFileOffset(0)
		, m_nCompressedSize(0)
		, m_nUncompressedSize(0)
	{
	}
	VPKChunkDescriptor_t(FileHandle_t hDirectoryFile);
	VPKChunkDescriptor_t(uint32_t nLoadFlags, uint16_t nTextureFlags,
		uint64_t nPackFileOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize);
};

//-----------------------------------------------------------------------------
// An asset packed into a VPK is represented as an entry block.
//-----------------------------------------------------------------------------
struct VPKEntryBlock_t
{
	// Crc32 for the uncompressed entry.
	uint32_t                         m_nFileCRC;
	uint16_t                         m_iPreloadSize;

	// Index of the pack file that contains this entry.
	uint16_t                         m_iPackFileIndex;

	// Vector of all the chunks of a given entry
	// (chunks have a size limit of 1 MiB, anything
	// over this limit is fragmented into smaller chunks).
	CUtlVector<VPKChunkDescriptor_t> m_Fragments;
	CUtlString                       m_EntryPath;

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

//-----------------------------------------------------------------------------
// The VPK directory file header.
//-----------------------------------------------------------------------------
struct VPKDirHeader_t
{
	uint32_t m_nHeaderMarker;  // File magic.
	uint16_t m_nMajorVersion;  // Vpk major version.
	uint16_t m_nMinorVersion;  // Vpk minor version.
	uint32_t m_nDirectorySize; // Directory tree size.
	uint32_t m_nSignatureSize; // Directory signature.

	VPKDirHeader_t()
	{
		m_nHeaderMarker = 0;
		m_nMajorVersion = 0;
		m_nMinorVersion = 0;
		m_nDirectorySize = 0;
		m_nSignatureSize = 0;
	}
};

//-----------------------------------------------------------------------------
// The VPK directory tree structure.
//-----------------------------------------------------------------------------
struct VPKDir_t
{
	VPKDirHeader_t               m_Header;
	CUtlString                   m_DirFilePath;
	CUtlVector<VPKEntryBlock_t>  m_EntryBlocks;

	// This set only contains packfile indices used
	// by the directory tree, notated as pak000_xxx.
	std::set<uint16_t>           m_PakFileIndices;
	bool m_bInitFailed;

	class CTreeBuilder
	{
	public:
		typedef std::map<std::string, std::list<VPKEntryBlock_t>> PathContainer_t;
		typedef std::map<std::string, PathContainer_t> TypeContainer_t;

		void BuildTree(const CUtlVector<VPKEntryBlock_t>& entryBlocks);
		int  WriteTree(FileHandle_t hDirectoryFile) const;

	private:
		TypeContainer_t m_FileTree;
	};

	VPKDir_t()
	{
		m_Header.m_nHeaderMarker = NULL; m_Header.m_nMajorVersion = NULL;
		m_Header.m_nMinorVersion = NULL; m_Header.m_nDirectorySize = NULL,
		m_Header.m_nSignatureSize = NULL; m_bInitFailed = false;
	};
	VPKDir_t(const CUtlString& svDirectoryFile);
	VPKDir_t(const CUtlString& svDirectoryFile, bool bSanitizeName);

	void Init(const CUtlString& svPath);
	inline bool Failed() const { return m_bInitFailed; }

	CUtlString StripLocalePrefix(const CUtlString& svDirectoryFile) const;
	CUtlString GetPackFileNameForIndex(uint16_t iPackFileIndex) const;

	void WriteHeader(FileHandle_t hDirectoryFile);
	void BuildDirectoryFile(const CUtlString& svDirectoryFile, const CUtlVector<VPKEntryBlock_t>& entryBlocks);
};

//-----------------------------------------------------------------------------
// Contains the VPK directory name, and the pack file name. Used for building
// the VPK file.
// !TODO[ AMOS ]: Remove this when patching is implemented!
//-----------------------------------------------------------------------------
struct VPKPair_t
{
	CUtlString m_PackName;
	CUtlString m_DirName;

	VPKPair_t(const char* svLocale, const char* svTarget, const char* svLevel, int nPatch);
};

//-----------------------------------------------------------------------------
// VPK utility class.
//-----------------------------------------------------------------------------
class CPackedStoreBuilder
{
public:
	void InitLzEncoder(const lzham_int32 maxHelperThreads = -1, const char* compressionLevel = "default");
	void InitLzDecoder(void);

	bool Deduplicate(const uint8_t* pEntryBuffer, VPKChunkDescriptor_t& descriptor, const size_t chunkIndex);

	void PackStore(const VPKPair_t& vpkPair, const char* workspaceName, const char* buildPath);
	void UnpackStore(const VPKDir_t& vpkDir, const char* workspaceName = "");

private:
	lzham_compress_params   m_Encoder; // LZham compression parameters.
	lzham_decompress_params m_Decoder; // LZham decompression parameters.
	std::unordered_map<string, const VPKChunkDescriptor_t&> m_ChunkHashMap;
};

CUtlString PackedStore_GetDirBaseName(const CUtlString& dirFileName);
CUtlString PackedStore_GetDirNameParts(const CUtlString& dirFileName, const int nCaptureGroup);
///////////////////////////////////////////////////////////////////////////////

#endif // PACKEDSTORE_H
