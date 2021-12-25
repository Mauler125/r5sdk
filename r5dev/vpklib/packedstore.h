#pragma once
#include "public/include/binstream.h"
#include "thirdparty/lzham/include/lzham.h"

constexpr unsigned int LIBRARY_PACKS = 2;
constexpr unsigned int LANGUAGE_PACKS = 11;
constexpr unsigned int RVPK_DICT_SIZE = 20;
constexpr unsigned int RVPK_DIR_MAGIC = 'Uª4';

const std::string DIR_LIBRARY_PREFIX[LIBRARY_PACKS] = { "server", "client" };
const std::string DIR_LOCALE_PREFIX[LANGUAGE_PACKS] = { "english", "french", "german", "italian", "japanese", "korean", "polish", "portuguese", "russian", "spanish", "tchinese" };

struct vpk_entry_h
{
	uint32_t m_nEntryFlags      {}; // Entry flags.
	uint16_t m_nTextureFlags    {}; // Texture flags (only used if the entry is a vtf).
	uint64_t m_nArchiveOffset   {}; // Offset in archive.
	uint64_t m_nCompressedSize  {}; // Compressed size of entry.
	uint64_t m_nUncompressedSize{}; // Uncompressed size of entry.
	bool     m_bIsCompressed  = false;

	vpk_entry_h(CIOStream* reader);
};

struct vpk_entry_block
{
	uint32_t                 m_nCrc32       {}; // Crc32 for the uncompressed block.
	uint16_t                 m_nPreloadBytes{}; // Preload bytes.
	uint16_t                 m_iArchiveIndex{}; // Index of the archive that contains this block.
	std::vector<vpk_entry_h> m_vvEntries    {}; // Vector of all the entries of a given block (entries have a size limit of 1 MiB, so anything over is split into separate entries within the same block).
	std::string              m_svBlockPath  {}; // Path to block within vpk.

	vpk_entry_block(CIOStream* reader, std::string path);
};

struct vpk_dir_h
{
	uint32_t                     m_nFileMagic   {}; // File magic.
	uint16_t                     m_nMajorVersion{}; // Vpk major version.
	uint16_t                     m_nMinorVersion{}; // Vpk minor version.
	uint32_t                     m_nTreeSize    {}; // Directory tree size.
	uint32_t                     m_nFileDataSize{}; // File data section size.
	std::vector<vpk_entry_block> m_vvEntryBlocks{}; // Vector of entry blocks.
	uint16_t                     m_iArchiveCount{}; // Highest archive index (archive count-1).
	std::vector<std::string>     m_vsvArchives  {}; // Vector of archive file names.
	std::string                  m_svDirPath    {}; // Path to vpk_dir file.

	vpk_dir_h(std::string path);
};

class CPackedStore
{
	std::vector<uint8_t>         m_vHashBuffer      {}; // Buffer for post decomp file validation.
	std::size_t                  m_nEntryCount      {}; // Entry per-block incrementor.
	lzham_uint32                 m_nAdler32_Internal{}; // Internal operation Adler32 file checksum.
	lzham_uint32                 m_nAdler32         {}; // Pre/post operation Adler32 file checksum.
	lzham_uint32                 m_nCrc32_Internal  {}; // Internal operation Crc32 file checksum.
	lzham_uint32                 m_nCrc32           {}; // Pre/post operation Crc32 file checksum.
	lzham_decompress_params      m_lzDecompParams   {}; // LZham decompression parameters.
	lzham_decompress_status_t    m_lzDecompStatus   {}; // LZham decompression results.

public:
	void InitLzParams();
	vpk_dir_h GetPackDirFile(std::string svPackDirFile);
	std::string GetPackChunkFile(std::string svPackDirFile, int iArchiveIndex);
	std::vector<vpk_entry_block> GetEntryBlocks(CIOStream* reader);
	std::string FormatBlockPath(std::string svName, std::string svPath, std::string svExtension);
	std::string StripLocalePrefix(std::string svPackDirFile);
	void UnpackAll(vpk_dir_h vpk, std::string svPathOut = "");
	void ValidateAdler32PostDecomp(std::string svDirAsset);
	void ValidateCRC32PostDecomp(std::string svDirAsset);
};
///////////////////////////////////////////////////////////////////////////////
extern CPackedStore* g_pPackedStore;
