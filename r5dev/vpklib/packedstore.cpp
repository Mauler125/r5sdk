//=============================================================================//
//
// Purpose: Valve Pak utility class.
//
//=============================================================================//
// packedstore.cpp
//
// Note: VPK's are created in pairs of a directory file and pack file(s).
// - <locale><target>_<level>.bsp.pak000_dir.vpk --> directory file.
// - <target>_<level>.bsp.pak000_<patch>.vpk ------> pack file.
// 
// The directory file contains the entire directory tree of the VPK. The
// filesystem essentially mounts this as additional paths to search through.
// 
// Each asset is an entry in the VPK directory (see 'VPKEntryBlock_t'), an asset
// contains at least 1 chunk (see 'VPKChunkDescriptor_t'). If an asset is larger
// than 'ENTRY_MAX_LEN', the asset will be carved into chunks of 'ENTRY_MAX_LEN'
// or smaller, as this is the size of the decompress buffer in the engine.
// 
// The VPK can be patched; the descriptor of this file would be adjusted as such
// that it would read the data from a different pack file containing the patched
// data. The only files that need to be shipped after a patch is the patched VPK
// directory file, and the additional pack file containing the patch. Untouched
// data is still getting read from the old pack file.
// 
/////////////////////////////////////////////////////////////////////////////////

#include "tier1/cvar.h"
#include "tier2/fileutils.h"
#include "mathlib/adler32.h"
#include "mathlib/crc32.h"
#include "mathlib/sha1.h"
#include "filesystem/filesystem.h"
#include "vpc/keyvalues.h"
#include "vpklib/packedstore.h"

static const std::regex s_DirFileRegex{ R"((?:.*\/)?([^_]*_)(.*)(.bsp.pak000_dir).*)" };
static const std::regex s_BlockFileRegex{ R"(pak000_([0-9]{3}))" };

//-----------------------------------------------------------------------------
// Purpose: initialize parameters for compression algorithm
//-----------------------------------------------------------------------------
void CPackedStore::InitLzCompParams(void)
{
	/*| PARAMETERS ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	m_lzCompParams.m_dict_size_log2     = VPK_DICT_SIZE;
	m_lzCompParams.m_level              = GetCompressionLevel();
	m_lzCompParams.m_compress_flags     = lzham_compress_flags::LZHAM_COMP_FLAG_DETERMINISTIC_PARSING;
	m_lzCompParams.m_max_helper_threads = fs_packedstore_max_helper_threads->GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: initialize parameters for decompression algorithm
//-----------------------------------------------------------------------------
void CPackedStore::InitLzDecompParams(void)
{
	/*| PARAMETERS ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	m_lzDecompParams.m_dict_size_log2   = VPK_DICT_SIZE;
	m_lzDecompParams.m_decompress_flags = lzham_decompress_flags::LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED;
	m_lzDecompParams.m_struct_size      = sizeof(lzham_decompress_params);
}

//-----------------------------------------------------------------------------
// Purpose: gets the LZHAM compression level
// output : lzham_compress_level
//-----------------------------------------------------------------------------
lzham_compress_level CPackedStore::GetCompressionLevel(void) const
{
	const char* pszLevel = fs_packedstore_compression_level->GetString();

	if(strcmp(pszLevel, "fastest") == NULL)
		return lzham_compress_level::LZHAM_COMP_LEVEL_FASTEST;
	else if (strcmp(pszLevel, "faster") == NULL)
		return lzham_compress_level::LZHAM_COMP_LEVEL_FASTER;
	else if (strcmp(pszLevel, "default") == NULL)
		return lzham_compress_level::LZHAM_COMP_LEVEL_DEFAULT;
	else if (strcmp(pszLevel, "better") == NULL)
		return lzham_compress_level::LZHAM_COMP_LEVEL_BETTER;
	else if (strcmp(pszLevel, "uber") == NULL)
		return lzham_compress_level::LZHAM_COMP_LEVEL_UBER;
	else
		return lzham_compress_level::LZHAM_COMP_LEVEL_DEFAULT;
}

//-----------------------------------------------------------------------------
// Purpose: obtains and returns the entry block to the vector
// Input  : &entryBlocks   - 
//          hDirectoryFile - 
// output : vector<VPKEntryBlock_t>
//-----------------------------------------------------------------------------
void CPackedStore::GetEntryBlocks(CUtlVector<VPKEntryBlock_t>& entryBlocks, FileHandle_t hDirectoryFile) const
{
	CUtlString fileName, filePath, fileExtension;

	while (!(fileExtension = FileSystem()->ReadString(hDirectoryFile)).IsEmpty())
	{
		while (!(filePath = FileSystem()->ReadString(hDirectoryFile)).IsEmpty())
		{
			while (!(fileName = FileSystem()->ReadString(hDirectoryFile)).IsEmpty())
			{
				filePath.AppendSlash();
				const CUtlString svFilePath = FormatEntryPath(filePath, fileName, fileExtension);

				entryBlocks.AddToTail(VPKEntryBlock_t(hDirectoryFile, svFilePath.Get()));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: scans the input directory and returns the values to the vector if path exists in manifest
// Input  : &entryValues   - 
//          *workspacePath - 
//          *dirFileName   - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CPackedStore::GetEntryValues(CUtlVector<VPKKeyValues_t>& entryValues, 
	const CUtlString& workspacePath, const CUtlString& dirFileName) const
{
	KeyValues* pManifestKV = GetManifest(workspacePath, GetLevelName(dirFileName));

	if (!pManifestKV)
	{
		Warning(eDLL_T::FS, "Invalid VPK build manifest KV; unable to parse entry list\n");
		return false;
	}

	CUtlVector<CUtlString> ignoreList;

	if (!GetIgnoreList(ignoreList, workspacePath))
	{
		Warning(eDLL_T::FS, "No ignore file provided; continuing build without...\n");
	}

	for (KeyValues* pSubKey = pManifestKV->GetFirstSubKey(); pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
	{
		const char* pszFileName = pSubKey->GetName();

		if (!VALID_CHARSTAR(pszFileName))
		{
			continue;
		}

		CUtlString fileName;
		fileName.Format("%s%s", workspacePath.Get(), pszFileName);

		if (ShouldPrune(fileName, ignoreList))
		{
			// Do not add to the build list.
			continue;
		}

		entryValues.AddToTail(VPKKeyValues_t(
			std::move(fileName),
			int16_t(pSubKey->GetInt("preloadSize", NULL)),
			pSubKey->GetInt("loadFlags", static_cast<uint32_t>(EPackedLoadFlags::LOAD_VISIBLE) | static_cast<uint32_t>(EPackedLoadFlags::LOAD_CACHE)),
			int16_t(pSubKey->GetInt("textureFlags", static_cast<uint16_t>(EPackedTextureFlags::TEXTURE_DEFAULT))),
			pSubKey->GetBool("useCompression", true),
			pSubKey->GetBool("deDuplicate", true))
		);
	}

	pManifestKV->DeleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets the parts of the directory file name
// Input  : &dirFileName   - 
//          nCaptureGroup  - (1 = locale + target, 2 = level)
// Output : part of directory file name as string
//-----------------------------------------------------------------------------
CUtlString CPackedStore::GetNameParts(const CUtlString& dirFileName, int nCaptureGroup) const
{
	std::cmatch regexMatches;
	std::regex_search(dirFileName.Get(), regexMatches, s_DirFileRegex);

	return regexMatches[nCaptureGroup].str().c_str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the level name from the directory file name
// Input  : &dirFileName - 
// Output : level name as string (e.g. "mp_rr_box")
//-----------------------------------------------------------------------------
CUtlString CPackedStore::GetLevelName(const CUtlString& dirFileName) const
{
	std::cmatch regexMatches;
	std::regex_search(dirFileName.Get(), regexMatches, s_DirFileRegex);

	CUtlString result;
	result.Format("%s%s", regexMatches[1].str().c_str(), regexMatches[2].str().c_str());

	return result;
}

//-----------------------------------------------------------------------------
// Purpose: gets the manifest file associated with the VPK name (must be freed after wards)
// Input  : &workspacePath - 
//          &manifestFile  - 
// Output : KeyValues (build manifest pointer)
//-----------------------------------------------------------------------------
KeyValues* CPackedStore::GetManifest(const CUtlString& workspacePath, const CUtlString& manifestFile) const
{
	CUtlString outPath;

	outPath.Format("%s%s%s.txt", workspacePath.Get(), "manifest/", manifestFile.Get());
	outPath.FixSlashes();

	KeyValues* pManifestKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, outPath.Get(), "PLATFORM");
	return pManifestKV;
}

//-----------------------------------------------------------------------------
// Purpose: gets the contents from the global ignore list (.vpkignore)
// Input  : &ignoreList    - 
//			&workspacePath - 
// Output : a string vector of ignored directories/files and extensions
//-----------------------------------------------------------------------------
bool CPackedStore::GetIgnoreList(CUtlVector<CUtlString>& ignoreList, const CUtlString& workspacePath) const
{
	CUtlString toIgnore;

	toIgnore.Format("%s%s", workspacePath.Get(), VPK_IGNORE_FILE);
	toIgnore.FixSlashes();

	FileHandle_t hIgnoreFile = FileSystem()->Open(toIgnore.Get(), "rt", "PLATFORM");
	if (!hIgnoreFile)
	{
		return false;
	}

	char szIgnore[MAX_PATH];

	while (FileSystem()->ReadLine(szIgnore, sizeof(szIgnore) - 1, hIgnoreFile))
	{
		if (!strstr(szIgnore, "//")) // Skip comments.
		{
			if (char* pEOL = strchr(szIgnore, '\n'))
			{
				// Null newline character.
				*pEOL = '\0';
				if (pEOL - szIgnore > 0)
				{
					// Null carriage return.
					if (*(pEOL - 1) == '\r')
					{
						*(pEOL - 1) = '\0';
					}
				}
			}

			ignoreList.AddToTail(szIgnore);
		}
	}

	FileSystem()->Close(hIgnoreFile);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: formats the file entry path
// Input  : &filePath - 
//          &fileName - 
//          &fileExt  - 
// Output : formatted entry path
//-----------------------------------------------------------------------------
CUtlString CPackedStore::FormatEntryPath(const CUtlString& filePath,
	const CUtlString& fileName, const CUtlString& fileExt) const
{
	CUtlString result;

	const char* pszFilePath = filePath.Get();
	const bool isRoot = pszFilePath[0] == ' ';

	result.Format("%s%s.%s", isRoot ? "" : pszFilePath,
		fileName.Get(), fileExt.Get());

	result.FixSlashes();
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: builds the VPK manifest file
// Input  : &entryBlocks   - 
//          &workspacePath - 
//          &manifestName  - 
//-----------------------------------------------------------------------------
void CPackedStore::BuildManifest(const CUtlVector<VPKEntryBlock_t>& entryBlocks, const CUtlString& workspacePath, const CUtlString& manifestName) const
{
	KeyValues kv("BuildManifest");
	KeyValues* pManifestKV = kv.FindKey("BuildManifest", true);

	FOR_EACH_VEC(entryBlocks, i)
	{
		const VPKEntryBlock_t& entry = entryBlocks[i];
		const VPKChunkDescriptor_t& descriptor = entry.m_Fragments[0];

		KeyValues* pEntryKV = pManifestKV->FindKey(entry.m_EntryPath.Get(), true);

		pEntryKV->SetInt("preloadSize", entry.m_iPreloadSize);
		pEntryKV->SetInt("loadFlags", descriptor.m_nLoadFlags);
		pEntryKV->SetInt("textureFlags", descriptor.m_nTextureFlags);
		pEntryKV->SetBool("useCompression", descriptor.m_nCompressedSize != descriptor.m_nUncompressedSize);
		pEntryKV->SetBool("deDuplicate", true);
	}

	CUtlString outPath;
	outPath.Format("%s%s%s.txt", workspacePath.Get(), "manifest/", manifestName.Get());
	outPath.FixSlashes();

	CUtlBuffer outBuf(int64_t(0), 0, CUtlBuffer::TEXT_BUFFER);
	kv.RecursiveSaveToFile(outBuf, 0);

	FileSystem()->CreateDirHierarchy(outPath.DirName().Get(), "PLATFORM");
	FileSystem()->WriteFile(outPath.Get(), "PLATFORM", outBuf);
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed CRC32 hash
// Input  : &assetPath - 
//        : nFileCRC   - 
//-----------------------------------------------------------------------------
void CPackedStore::ValidateCRC32PostDecomp(const CUtlString& assetPath, const uint32_t nFileCRC)
{
	FileHandle_t hAsset = FileSystem()->Open(assetPath.Get(), "rb", "PLATFORM");
	if (!hAsset)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, assetPath.Get());
		return;
	}

	uint32_t nLen = FileSystem()->Size(hAsset);
	std::unique_ptr<uint8_t[]> pBuf(new uint8_t[nLen]);

	FileSystem()->Read(pBuf.get(), nLen, hAsset);
	FileSystem()->Close(hAsset);

	uint32_t nCrc32 = crc32::update(NULL, pBuf.get(), nLen);
	if (nCrc32 != nFileCRC)
	{
		Warning(eDLL_T::FS, "Computed checksum '0x%lX' doesn't match expected checksum '0x%lX'. File may be corrupt!\n", nCrc32, nFileCRC);
	}
}

//-----------------------------------------------------------------------------
// Purpose: attempts to deduplicate a chunk of data by comparing it to existing chunks
// Input  : *pEntryBuffer - 
//          &descriptor   - 
//          chunkIndex    - 
// Output : true if the chunk was deduplicated, false otherwise
//-----------------------------------------------------------------------------
bool CPackedStore::Deduplicate(const uint8_t* pEntryBuffer, VPKChunkDescriptor_t& descriptor, const size_t chunkIndex)
{
	string entryHash(reinterpret_cast<const char*>(pEntryBuffer), descriptor.m_nUncompressedSize);
	entryHash = sha1(entryHash);

	auto p = m_ChunkHashMap.insert({ entryHash.c_str(), descriptor });
	if (!p.second) // Map to existing chunk to avoid having copies of the same data.
	{
		DevMsg(eDLL_T::FS, "Mapping chunk '%zu' ('%s') to existing chunk at '0x%llx'\n",
			chunkIndex, entryHash.c_str(), p.first->second.m_nPackFileOffset);
		descriptor = p.first->second;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: determines whether the file should be pruned from the build list
// Input  : &filePath   - 
//          &ignoreList - 
// Output : true if it should be pruned, false otherwise
//-----------------------------------------------------------------------------
bool CPackedStore::ShouldPrune(const CUtlString& filePath, CUtlVector<CUtlString>& ignoreList) const
{
	if (!V_IsValidPath(filePath.Get()))
	{
		return true;
	}

	FOR_EACH_VEC(ignoreList, j)
	{
		const CUtlString& ignoreEntry = ignoreList[j];

		if (ignoreEntry.IsEqual_CaseInsensitive(filePath.Get()))
		{
			return true;
		}
	}

	FileHandle_t fileHandle = FileSystem()->Open(filePath.Get(), "rb", "PLATFORM");

	if (fileHandle)
	{
		const int nSize = FileSystem()->Size(fileHandle);

		if (!nSize)
		{
			Warning(eDLL_T::FS, "File '%s' listed in build manifest appears empty or truncated\n", filePath.Get());
			FileSystem()->Close(fileHandle);

			return true;
		}

		FileSystem()->Close(fileHandle);
	}
	else
	{
		Warning(eDLL_T::FS, "File '%s' listed in build manifest couldn't be opened\n", filePath.Get());
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: packs all files from workspace path into VPK file
// Input  : &vpkPair       - 
//          *workspaceName - 
//          *buildPath     - 
//-----------------------------------------------------------------------------
void CPackedStore::PackWorkspace(const VPKPair_t& vpkPair, const char* workspaceName, const char* buildPath)
{
	CUtlString workspacePath(workspaceName);
	workspacePath.AppendSlash();
	workspacePath.FixSlashes();

	CUtlString packFilePath;
	CUtlString dirFilePath;
	
	packFilePath.Format("%s%s", buildPath, vpkPair.m_PackName.Get());
	dirFilePath.Format("%s%s", buildPath, vpkPair.m_DirName.Get());

	FileHandle_t hPackFile = FileSystem()->Open(packFilePath.Get(), "wb", "GAME");
	if (!hPackFile)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, packFilePath.Get());
		return;
	}

	std::unique_ptr<uint8_t[]> pEntryBuffer(new uint8_t[ENTRY_MAX_LEN]);

	if (!pEntryBuffer)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to allocate memory for entry buffer!\n", __FUNCTION__);
		FileSystem()->Close(hPackFile);
		return;
	}

	CUtlVector<VPKKeyValues_t> entryValues;
	CUtlVector<VPKEntryBlock_t> entryBlocks;

	if (!GetEntryValues(entryValues, workspacePath, vpkPair.m_DirName))
	{
		FileSystem()->Close(hPackFile);
		return;
	}

	uint64_t nSharedTotal = NULL;
	uint32_t nSharedCount = NULL;

	FOR_EACH_VEC(entryValues, i)
	{
		const VPKKeyValues_t& entryValue = entryValues[i];
		FileHandle_t hAsset = FileSystem()->Open(entryValue.m_EntryPath.Get(), "rb", "PLATFORM");

		if (!hAsset)
		{
			Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, entryValue.m_EntryPath.Get());
			continue;
		}

		const char* szDestPath = (entryValue.m_EntryPath.Get() + workspacePath.Length());
		if (PATHSEPARATOR(szDestPath[0]))
		{
			szDestPath++;
		}

		uint32_t nLen = FileSystem()->Size(hAsset);
		std::unique_ptr<uint8_t[]> pBuf(new uint8_t[nLen]);

		FileSystem()->Read(pBuf.get(), nLen, hAsset);
		FileSystem()->Seek(hAsset, 0, FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);

		DevMsg(eDLL_T::FS, "Packing entry '%i' ('%s')\n", i, szDestPath);
		entryBlocks.AddToTail(VPKEntryBlock_t(
			pBuf.get(),
			nLen,
			FileSystem()->Tell(hPackFile),
			entryValue.m_iPreloadSize,
			0,
			entryValue.m_nLoadFlags,
			entryValue.m_nTextureFlags,
			CUtlString(szDestPath)));

		VPKEntryBlock_t& entryBlock = entryBlocks[i];

		FOR_EACH_VEC(entryBlock.m_Fragments, j)
		{
			VPKChunkDescriptor_t& descriptor = entryBlock.m_Fragments[j];

			FileSystem()->Read(pEntryBuffer.get(), int(descriptor.m_nCompressedSize), hAsset);
			descriptor.m_nPackFileOffset = FileSystem()->Tell(hPackFile);

			if (entryValue.m_bDeduplicate && Deduplicate(pEntryBuffer.get(), descriptor, j))
			{
				nSharedTotal += descriptor.m_nCompressedSize;
				nSharedCount++;

				// Data was deduplicated.
				continue;
			}

			if (entryValue.m_bUseCompression)
			{
				lzham_compress_status_t lzCompStatus = lzham_compress_memory(&m_lzCompParams, pEntryBuffer.get(), &descriptor.m_nCompressedSize, pEntryBuffer.get(),
					descriptor.m_nUncompressedSize, nullptr);

				if (lzCompStatus != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
				{
					Warning(eDLL_T::FS, "Status '%d' for chunk '%i' within entry '%i' in block '%hu' (chunk packed without compression)\n",
						lzCompStatus, j, i, entryBlocks[i].m_iPackFileIndex);

					descriptor.m_nCompressedSize = descriptor.m_nUncompressedSize;
				}
			}
			else // Write data uncompressed.
			{
				descriptor.m_nCompressedSize = descriptor.m_nUncompressedSize;
			}

			FileSystem()->Write(pEntryBuffer.get(), int(descriptor.m_nCompressedSize), hPackFile);
		}

		FileSystem()->Close(hAsset);
	}

	DevMsg(eDLL_T::FS, "*** Build block totaling '%zu' bytes with '%zu' shared bytes among '%zu' chunks\n", FileSystem()->Tell(hPackFile), nSharedTotal, nSharedCount);
	FileSystem()->Close(hPackFile);

	m_ChunkHashMap.clear();

	VPKDir_t vDirectory;
	vDirectory.BuildDirectoryFile(dirFilePath, entryBlocks);
}

//-----------------------------------------------------------------------------
// Purpose: rebuilds manifest and extracts all files from specified VPK file
// Input  : &vpkDirectory  - 
//          &workspaceName - 
//-----------------------------------------------------------------------------
void CPackedStore::UnpackWorkspace(const VPKDir_t& vpkDir, const char* workspaceName)
{
	CUtlString workspacePath(workspaceName);
	workspacePath.AppendSlash();
	workspacePath.FixSlashes();

	if (vpkDir.m_Header.m_nHeaderMarker != VPK_HEADER_MARKER ||
		vpkDir.m_Header.m_nMajorVersion != VPK_MAJOR_VERSION ||
		vpkDir.m_Header.m_nMinorVersion != VPK_MINOR_VERSION)
	{
		Error(eDLL_T::FS, NO_ERROR, "Unsupported VPK directory file (invalid header criteria)\n");
		return;
	}

	std::unique_ptr<uint8_t[]> pDestBuffer(new uint8_t[ENTRY_MAX_LEN]);
	std::unique_ptr<uint8_t[]> pSourceBuffer(new uint8_t[ENTRY_MAX_LEN]);

	if (!pDestBuffer || !pSourceBuffer)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to allocate memory for entry buffer!\n", __FUNCTION__);
		return;
	}

	BuildManifest(vpkDir.m_EntryBlocks, workspacePath, GetLevelName(vpkDir.m_DirFilePath));
	const CUtlString basePath = vpkDir.m_DirFilePath.StripFilename(false);

	FOR_EACH_VEC(vpkDir.m_PackFiles, i)
	{
		const CUtlString packFile = basePath + vpkDir.m_PackFiles[i];

		// Read from each pack file.
		FileHandle_t hPackFile = FileSystem()->Open(packFile.Get(), "rb", "GAME");
		if (!hPackFile)
		{
			Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, packFile.Get());
			continue;
		}

		FOR_EACH_VEC(vpkDir.m_EntryBlocks, j)
		{
			const VPKEntryBlock_t& entryBlock = vpkDir.m_EntryBlocks[j];

			if (entryBlock.m_iPackFileIndex != uint16_t(i))
			{
				// Chunk doesn't belongs to this block.
				continue;
			}

			CUtlString filePath;
			filePath.Format("%s%s", workspacePath.Get(), entryBlock.m_EntryPath.Get());

			FileSystem()->CreateDirHierarchy(filePath.DirName().Get(), "PLATFORM");
			FileHandle_t hAsset = FileSystem()->Open(filePath.Get(), "wb", "PLATFORM");

			if (!hAsset)
			{
				Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, filePath.Get());
				continue;
			}

			DevMsg(eDLL_T::FS, "Unpacking entry '%i' from block '%i' ('%s')\n", j, i, entryBlock.m_EntryPath.Get());

			FOR_EACH_VEC(entryBlock.m_Fragments, k)
			{
				const VPKChunkDescriptor_t& fragment = entryBlock.m_Fragments[k];

				FileSystem()->Seek(hPackFile, int(fragment.m_nPackFileOffset), FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);
				FileSystem()->Read(pSourceBuffer.get(), int(fragment.m_nCompressedSize), hPackFile);

				if (fragment.m_nCompressedSize == fragment.m_nUncompressedSize) // Data is not compressed.
				{
					FileSystem()->Write(pSourceBuffer.get(), int(fragment.m_nUncompressedSize), hAsset);
					continue;
				}

				size_t nDstLen = ENTRY_MAX_LEN;
				assert(fragment.m_nCompressedSize <= nDstLen);

				if (fragment.m_nCompressedSize > nDstLen)
					break; // Corrupt or invalid chunk descriptor.

				lzham_decompress_status_t lzDecompStatus = lzham_decompress_memory(&m_lzDecompParams, pDestBuffer.get(),
					&nDstLen, pSourceBuffer.get(), fragment.m_nCompressedSize, nullptr);

				if (lzDecompStatus != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
				{
					Error(eDLL_T::FS, NO_ERROR, "Status '%d' for chunk '%zu' within entry '%zu' in block '%hu' (chunk not decompressed)\n",
						lzDecompStatus, k, j, i);
				}
				else // If successfully decompressed, write to file.
				{
					FileSystem()->Write(pDestBuffer.get(), int(nDstLen), hAsset);
				}
			}

			FileSystem()->Close(hAsset);
			ValidateCRC32PostDecomp(filePath, entryBlock.m_nFileCRC);
		}
		FileSystem()->Close(hPackFile);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKKeyValues_t' memory constructor
// Input  : &entryPath      - 
//          iPreloadSize    - 
//          nLoadFlags      - 
//          nTextureFlags   - 
//          bUseCompression - 
//          bDeduplicate    - 
//-----------------------------------------------------------------------------
VPKKeyValues_t::VPKKeyValues_t(const CUtlString& entryPath, uint16_t iPreloadSize,
	uint32_t nLoadFlags, uint16_t nTextureFlags, bool bUseCompression, bool bDeduplicate)
{
	m_EntryPath = entryPath;
	m_iPreloadSize = iPreloadSize;
	m_nLoadFlags = nLoadFlags;
	m_nTextureFlags = nTextureFlags;
	m_bUseCompression = bUseCompression;
	m_bDeduplicate = bDeduplicate;
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' file constructor
// Input  : hDirFile    - 
//          *pEntryPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(FileHandle_t hDirFile, const char* pEntryPath)
{
	m_EntryPath = pEntryPath; // Set the entry path.

	m_EntryPath.FixSlashes(); // Fix slashes and remove space character representing VPK root.
	m_EntryPath = m_EntryPath.Replace(" " CORRECT_PATH_SEPARATOR_S, "");

	FileSystem()->Read(&m_nFileCRC, sizeof(uint32_t), hDirFile);       //
	FileSystem()->Read(&m_iPreloadSize, sizeof(uint16_t), hDirFile);   //
	FileSystem()->Read(&m_iPackFileIndex, sizeof(uint16_t), hDirFile); //

	uint16_t nMarker = 0;
	do // Loop through all chunks in the entry and add to list.
	{
		VPKChunkDescriptor_t entry(hDirFile);
		m_Fragments.AddToTail(entry);

		FileSystem()->Read(&nMarker, sizeof(nMarker), hDirFile);

	} while (nMarker != static_cast<uint16_t>(PACKFILEINDEX_END));
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' memory constructor
// Input  : *pData         - 
//          nLen           - 
//          nOffset        - 
//          iPreloadSize   - 
//          iPackFileIndex - 
//          nLoadFlags     - 
//          nTextureFlags  - 
//          &pEntryPath    - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(const uint8_t* pData, size_t nLen, int64_t nOffset, uint16_t iPreloadSize, 
	uint16_t iPackFileIndex, uint32_t nLoadFlags, uint16_t nTextureFlags, const char* pEntryPath)
{
	m_nFileCRC = crc32::update(NULL, pData, nLen);
	m_iPreloadSize = iPreloadSize;
	m_iPackFileIndex = iPackFileIndex;
	m_EntryPath = pEntryPath;

	size_t nFragmentCount = (nLen + ENTRY_MAX_LEN - 1) / ENTRY_MAX_LEN;
	size_t nFileSize = nLen;
	int64_t nCurrentOffset = nOffset;

	for (size_t i = 0; i < nFragmentCount; i++) // Fragment data into 1 MiB chunks.
	{
		size_t nSize = std::min<uint64_t>(ENTRY_MAX_LEN, nFileSize);
		nFileSize -= nSize;
		m_Fragments.AddToTail(VPKChunkDescriptor_t(nLoadFlags, nTextureFlags, nCurrentOffset, nSize, nSize));
		nCurrentOffset += nSize;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' file constructor
// Input  : hDirFile - 
//-----------------------------------------------------------------------------
VPKChunkDescriptor_t::VPKChunkDescriptor_t(FileHandle_t hDirFile)
{
	FileSystem()->Read(&m_nLoadFlags, sizeof(uint32_t), hDirFile);        //
	FileSystem()->Read(&m_nTextureFlags, sizeof(uint16_t), hDirFile);     //
	FileSystem()->Read(&m_nPackFileOffset, sizeof(uint64_t), hDirFile);   //
	FileSystem()->Read(&m_nCompressedSize, sizeof(uint64_t), hDirFile);   //
	FileSystem()->Read(&m_nUncompressedSize, sizeof(uint64_t), hDirFile); //
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' memory constructor
// Input  : nLoadFlags        - 
//          nTextureFlags     - 
//          nArchiveOffset    - 
//          nCompressedSize   - 
//          nUncompressedSize - 
//-----------------------------------------------------------------------------
VPKChunkDescriptor_t::VPKChunkDescriptor_t(uint32_t nLoadFlags, uint16_t nTextureFlags, 
	uint64_t nPackFileOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize)
{
	m_nLoadFlags = nLoadFlags;
	m_nTextureFlags = nTextureFlags;
	m_nPackFileOffset = nPackFileOffset;

	m_nCompressedSize = nCompressedSize;
	m_nUncompressedSize = nUncompressedSize;
}

//-----------------------------------------------------------------------------
// Purpose: builds a valid file name for the VPK
// Input  : *pLocale - 
//          *pTarget - 
//          *pLevel  - 
//          nPatch   - 
// Output : a vpk file pair (block and directory file names)
//-----------------------------------------------------------------------------
VPKPair_t::VPKPair_t(const char* pLocale, const char* pTarget, const char* pLevel, int nPatch)
{
	bool bFoundLocale = false;

	for (size_t i = 0; i < SDK_ARRAYSIZE(DIR_LOCALE); i++)
	{
		if (V_strcmp(pLocale, DIR_LOCALE[i]) == NULL)
		{
			bFoundLocale = true;
		}
	}

	if (!bFoundLocale)
	{
		Warning(eDLL_T::FS, "Locale '%s' not supported; using default '%s'\n", pLocale, DIR_LOCALE[0]);
		pLocale = DIR_LOCALE[0];
	}

	bool bFoundTarget = false;

	for (size_t i = 0; i < SDK_ARRAYSIZE(DIR_TARGET); i++)
	{
		if (V_strcmp(pTarget, DIR_TARGET[i]) == NULL)
		{
			bFoundTarget = true;
		}
	}

	if (!bFoundTarget)
	{
		Warning(eDLL_T::FS, "Target '%s' not supported; using default '%s'\n", pTarget, DIR_TARGET[0]);
		pTarget = DIR_TARGET[0];
	}

	m_PackName.Format("%s_%s.bsp.pak000_%03d.vpk", pTarget, pLevel, nPatch);
	m_DirName.Format("%s%s_%s.bsp.pak000_dir.vpk", pLocale, pTarget, pLevel);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &dirFilePath - 
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const CUtlString& dirFilePath)
{
	Init(dirFilePath);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor with sanitation
// Input  : &dirFilePath  - 
//          bSanitizeName - retrieve the directory file name from block name
// Output : VPKDir_t
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const CUtlString& dirFilePath, bool bSanitizeName)
{
	if (!bSanitizeName)
	{
		Init(dirFilePath);
		return;
	}

	std::cmatch regexMatches;
	std::regex_search(dirFilePath.String(), regexMatches, s_BlockFileRegex);

	if (regexMatches.empty())
	{
		Init(dirFilePath);
		return;
	}

	CUtlString sanitizedName = dirFilePath;
	sanitizedName = sanitizedName.Replace(regexMatches[0].str().c_str(), "pak000_dir");

	bool bHasLocale = false;

	for (size_t i = 0; i < SDK_ARRAYSIZE(DIR_LOCALE); i++)
	{
		if (sanitizedName.Find(DIR_LOCALE[i]) != -1)
		{
			bHasLocale = true;
			break;
		}
	}

	if (!bHasLocale) // Only sanitize if no locale was provided.
	{
		CUtlString packDirPrefix;
		packDirPrefix.Append(DIR_LOCALE[0]);

		for (size_t i = 0; i < SDK_ARRAYSIZE(DIR_TARGET); i++)
		{
			const char* targetName = DIR_TARGET[i];

			if (sanitizedName.Find(targetName) != -1)
			{
				packDirPrefix.Append(targetName);
				packDirPrefix = packDirPrefix.Replace(targetName, packDirPrefix);

				break;
			}
		}
	}

	Init(sanitizedName);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &dirFilePath - 
//-----------------------------------------------------------------------------
void VPKDir_t::Init(const CUtlString& dirFilePath)
{
	// Create stream to read from each pack file.
	FileHandle_t hDirFile = FileSystem()->Open(dirFilePath.Get(), "rb", "GAME");
	if (!hDirFile)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, dirFilePath.Get());
		return;
	}

	FileSystem()->Read(&m_Header.m_nHeaderMarker, sizeof(uint32_t), hDirFile);
	FileSystem()->Read(&m_Header.m_nMajorVersion, sizeof(uint16_t), hDirFile);  //
	FileSystem()->Read(&m_Header.m_nMinorVersion, sizeof(uint16_t), hDirFile);  //
	FileSystem()->Read(&m_Header.m_nDirectorySize, sizeof(uint32_t), hDirFile); //
	FileSystem()->Read(&m_Header.m_nSignatureSize, sizeof(uint32_t), hDirFile); //

	g_pPackedStore->GetEntryBlocks(m_EntryBlocks, hDirFile);

	m_DirFilePath = dirFilePath; // Set path to vpk directory file.
	m_PackFileCount = 0;

	FOR_EACH_VEC(m_EntryBlocks, i)
	{
		const VPKEntryBlock_t& entryBlock = m_EntryBlocks[i];

		if (entryBlock.m_iPackFileIndex > m_PackFileCount)
		{
			m_PackFileCount = entryBlock.m_iPackFileIndex;
		}
	}

	for (uint16_t i = 0; i < m_PackFileCount + 1; i++)
	{
		m_PackFiles.AddToTail(GetPackFile(dirFilePath, i));
	}

	FileSystem()->Close(hDirFile);
}

//-----------------------------------------------------------------------------
// Purpose: formats pack file path for specific directory file
// Input  : &directoryPath - 
//          iPackFileIndex - 
// output : string
//-----------------------------------------------------------------------------
CUtlString VPKDir_t::GetPackFile(const CUtlString& directoryPath, uint16_t iPackFileIndex) const
{
	CUtlString packChunkName = StripLocalePrefix(directoryPath);
	CUtlString packChunkIndex;

	packChunkIndex.Format("pak000_%03d", iPackFileIndex);
	packChunkName = packChunkName.Replace("pak000_dir", packChunkIndex.Get());

	return packChunkName;
}

//-----------------------------------------------------------------------------
// Purpose: strips locale prefix from file path
// Input  : &directoryPath - 
// Output : directory filename without locale prefix
//-----------------------------------------------------------------------------
CUtlString VPKDir_t::StripLocalePrefix(const CUtlString& directoryPath) const
{
	CUtlString fileName = directoryPath.UnqualifiedFilename();

	for (size_t i = 0; i < SDK_ARRAYSIZE(DIR_LOCALE); i++)
	{
		fileName = fileName.Replace(DIR_LOCALE[i], "");
	}

	return fileName;
}

//-----------------------------------------------------------------------------
// Purpose: writes the vpk directory header
// Input  : hDirectoryFile - 
//-----------------------------------------------------------------------------
void VPKDir_t::WriteHeader(FileHandle_t hDirectoryFile) const
{
	FileSystem()->Write(&m_Header.m_nHeaderMarker, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&m_Header.m_nMajorVersion, sizeof(uint16_t), hDirectoryFile);
	FileSystem()->Write(&m_Header.m_nMinorVersion, sizeof(uint16_t), hDirectoryFile);
	FileSystem()->Write(&m_Header.m_nDirectorySize, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&m_Header.m_nSignatureSize, sizeof(uint32_t), hDirectoryFile);
}

//-----------------------------------------------------------------------------
// Purpose: writes the directory tree size
// Input  : hDirectoryFile - 
//-----------------------------------------------------------------------------
void VPKDir_t::WriteTreeSize(FileHandle_t hDirectoryFile) const
{
	FileSystem()->Seek(hDirectoryFile, offsetof(VPKDir_t, m_Header.m_nDirectorySize), FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);
	FileSystem()->Write(&m_Header.m_nDirectorySize, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint32_t), hDirectoryFile);
}

//-----------------------------------------------------------------------------
// Purpose: writes the vpk chunk descriptors
// Input  : hDirectoryFile - 
//          &vMap          - 
// Output : number of descriptors written
//-----------------------------------------------------------------------------
uint64_t VPKDir_t::WriteDescriptor(FileHandle_t hDirectoryFile,
	std::map<CUtlString, std::map<CUtlString, std::list<VPKEntryBlock_t>>>& vMap) const
{
	uint64_t nDescriptors = NULL;

	for (auto& iKeyValue : vMap)
	{
		FileSystem()->Write(iKeyValue.first.Get(), int(iKeyValue.first.Length() + 1), hDirectoryFile);
		for (auto& jKeyValue : iKeyValue.second)
		{
			FileSystem()->Write(jKeyValue.first.Get(), int(jKeyValue.first.Length() + 1), hDirectoryFile);
			for (auto& vEntry : jKeyValue.second)
			{
				CUtlString entryPath = vEntry.m_EntryPath.UnqualifiedFilename().StripExtension();

				FileSystem()->Write(entryPath.Get(), int(entryPath.Length() + 1), hDirectoryFile);

				FileSystem()->Write(&vEntry.m_nFileCRC, sizeof(uint32_t), hDirectoryFile);
				FileSystem()->Write(&vEntry.m_iPreloadSize, sizeof(uint16_t), hDirectoryFile);
				FileSystem()->Write(&vEntry.m_iPackFileIndex, sizeof(uint16_t), hDirectoryFile);

				FOR_EACH_VEC(vEntry.m_Fragments, i)
				{
					/*Write chunk descriptor*/
					const VPKChunkDescriptor_t& descriptor = vEntry.m_Fragments[i];

					FileSystem()->Write(&descriptor.m_nLoadFlags, sizeof(uint32_t), hDirectoryFile);
					FileSystem()->Write(&descriptor.m_nTextureFlags, sizeof(uint16_t), hDirectoryFile);
					FileSystem()->Write(&descriptor.m_nPackFileOffset, sizeof(uint64_t), hDirectoryFile);
					FileSystem()->Write(&descriptor.m_nCompressedSize, sizeof(uint64_t), hDirectoryFile);
					FileSystem()->Write(&descriptor.m_nUncompressedSize, sizeof(uint64_t), hDirectoryFile);

					if (i != (vEntry.m_Fragments.Count() - 1))
					{
						FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint16_t), hDirectoryFile);
					}
					else // Mark end of entry.
					{
						FileSystem()->Write(&PACKFILEINDEX_END, sizeof(uint16_t), hDirectoryFile);
					}
					nDescriptors++;
				}
			}
			FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint8_t), hDirectoryFile);
		}
		FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint8_t), hDirectoryFile);
	}
	FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint8_t), hDirectoryFile);

	return nDescriptors;
}

//-----------------------------------------------------------------------------
// Purpose: builds the vpk directory tree
// Input  : &vEntryBlocks - 
//          &vMap         - 
//-----------------------------------------------------------------------------
void VPKDir_t::BuildDirectoryTree(const CUtlVector<VPKEntryBlock_t>& entryBlocks,
	std::map<CUtlString, std::map<CUtlString, std::list<VPKEntryBlock_t>>>& vMap) const
{
	FOR_EACH_VEC(entryBlocks, i)
	{
		const VPKEntryBlock_t& entryBlock = entryBlocks[i];

		CUtlString fileExt = entryBlock.m_EntryPath.GetExtension();
		CUtlString filePath = entryBlock.m_EntryPath.DirName();

		if (!filePath.IsEmpty() && filePath[0] == '.')
		{
			filePath = " "; // Has to be padded with a space character if empty [root].
		}
		if (!vMap.count(fileExt))
		{
			vMap.insert({ fileExt, std::map<CUtlString, std::list<VPKEntryBlock_t>>() });
		}
		if (!vMap[fileExt].count(filePath))
		{
			vMap[fileExt].insert({ filePath, std::list<VPKEntryBlock_t>() });
		}
		vMap[fileExt][filePath].push_back(entryBlock);
	}
}

//-----------------------------------------------------------------------------
// Purpose: builds the vpk directory file
// Input  : &svDirectoryPath - 
//          &vEntryBlocks    - 
//-----------------------------------------------------------------------------
void VPKDir_t::BuildDirectoryFile(const CUtlString& directoryPath, const CUtlVector<VPKEntryBlock_t>& entryBlocks)
{
	FileHandle_t hDirectoryFile = FileSystem()->Open(directoryPath.Get(), "wb", "GAME");
	if (!hDirectoryFile)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, directoryPath.Get());
		return;
	}

	auto vMap = std::map<CUtlString, std::map<CUtlString, std::list<VPKEntryBlock_t>>>();
	BuildDirectoryTree(entryBlocks, vMap);

	WriteHeader(hDirectoryFile);
	uint64_t nDescriptors = WriteDescriptor(hDirectoryFile, vMap);

	m_Header.m_nDirectorySize = static_cast<uint32_t>(FileSystem()->Tell(hDirectoryFile) - sizeof(VPKDirHeader_t));
	WriteTreeSize(hDirectoryFile);

	FileSystem()->Close(hDirectoryFile);
	DevMsg(eDLL_T::FS, "*** Build directory totaling '%zu' bytes with '%zu' entries and '%zu' descriptors\n",
		size_t(sizeof(VPKDirHeader_t) + m_Header.m_nDirectorySize), entryBlocks.Count(), nDescriptors);
}
//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CPackedStore* g_pPackedStore = new CPackedStore();
