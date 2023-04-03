//=============================================================================//
//
// Purpose: Valve Pak utility class.
//
//=============================================================================//
// packedstore.cpp
//
// Note: VPK's are created in pairs of a directory file and pack file(s).
// - <locale><target>_<level>.bsp.pak000_dir.vpk --> directory file.
// - <target>_<level>.bsp.pak000_<patch>.vpk --> pack file.
// 
// - Assets larger than 1MiB are fragmented into chunks of 1MiB or smaller (ENTRY_MAX_LEN).
// - A VPK directory file could be patched up to 512 times before full rebuild is required.
// 
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "mathlib/adler32.h"
#include "mathlib/crc32.h"
#include "mathlib/sha1.h"
#include "filesystem/filesystem.h"
#include "vpc/keyvalues.h"
#include "vpklib/packedstore.h"

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
// Input  : hDirectoryFile - 
// output : vector<VPKEntryBlock_t>
//-----------------------------------------------------------------------------
vector<VPKEntryBlock_t> CPackedStore::GetEntryBlocks(FileHandle_t hDirectoryFile) const
{
	string svName, svPath, svExtension;
	vector<VPKEntryBlock_t> vEntryBlocks;
	while (!(svExtension = FileSystem()->ReadString(hDirectoryFile)).empty())
	{
		while (!(svPath = FileSystem()->ReadString(hDirectoryFile)).empty())
		{
			while (!(svName = FileSystem()->ReadString(hDirectoryFile)).empty())
			{
				const string svFilePath = FormatEntryPath(svPath, svName, svExtension);
				vEntryBlocks.push_back(VPKEntryBlock_t(hDirectoryFile, svFilePath));
			}
		}
	}
	return vEntryBlocks;
}

//-----------------------------------------------------------------------------
// Purpose: scans the input directory and returns the paths to the vector
// Input  : &svWorkspace - 
// Output : vpk keyvalues vector of all existing entry paths
//-----------------------------------------------------------------------------
vector<VPKKeyValues_t> CPackedStore::GetEntryValues(const string& svWorkspace) const
{
	vector<VPKKeyValues_t> vEntryValues;
	vector<string> vIgnoredList = GetIgnoreList(svWorkspace);

	fs::recursive_directory_iterator dir(svWorkspace), end;
	while (dir != end)
	{
		const vector<string>::iterator it = std::find(vIgnoredList.begin(), vIgnoredList.end(),
			GetExtension(dir->path().filename().u8string(), true, true));
		if (it != vIgnoredList.end())
		{
			dir.disable_recursion_pending(); // Skip all ignored folders and extensions.
		}
		else if (fs::file_size(*dir) > 0) // Empty files are not supported.
		{
			const string svEntryPath = dir->path().u8string();
			if (!GetExtension(svEntryPath).empty())
			{
				vEntryValues.push_back(VPKKeyValues_t(ConvertToUnixPath(svEntryPath)));
			}
		}
		dir++;
	}
	return vEntryValues;
}

//-----------------------------------------------------------------------------
// Purpose: scans the input directory and returns the values to the vector if path exists in manifest
// Input  : &svWorkspace - 
//          *pManifestKV - 
// Output : vpk keyvalues vector of all existing and included entry paths
//-----------------------------------------------------------------------------
vector<VPKKeyValues_t> CPackedStore::GetEntryValues(const string& svWorkspace, KeyValues* pManifestKV) const
{
	vector<VPKKeyValues_t> vEntryValues;

	if (!pManifestKV)
	{
		Warning(eDLL_T::FS, "Invalid VPK build manifest KV; unable to parse entry list\n");
		return vEntryValues;
	}

	vector<string> vIgnoredList = GetIgnoreList(svWorkspace);
	fs::recursive_directory_iterator dir(svWorkspace), end;

	while (dir != end)
	{
		const vector<string>::iterator it = std::find(vIgnoredList.begin(), vIgnoredList.end(), 
			GetExtension(dir->path().filename().u8string(), true, true));
		if (it != vIgnoredList.end())
		{
			dir.disable_recursion_pending(); // Skip all ignored folders and extensions.
			continue;
		}
		const string svFullPath = ConvertToWinPath(dir->path().u8string());
		if (!GetExtension(svFullPath).empty())
		{
			// Remove workspace path by offsetting it by its size.
			const char* pszEntry = (svFullPath.c_str() + svWorkspace.length());
			KeyValues* pEntryKV = pManifestKV->FindKey(pszEntry);

			if (pEntryKV)
			{
				if (!file_size(*dir)) // Empty files are not supported.
				{
					Warning(eDLL_T::FS, "File '%s' listed in build manifest appears truncated\n", dir->path().relative_path().u8string().c_str());
				}
				else
				{
					vEntryValues.push_back(VPKKeyValues_t(
						ConvertToUnixPath(svFullPath),
						int16_t(pEntryKV->GetInt("preloadSize", NULL)),
						pEntryKV->GetInt("loadFlags", static_cast<uint32_t>(EPackedLoadFlags::LOAD_VISIBLE) | static_cast<uint32_t>(EPackedLoadFlags::LOAD_CACHE)),
						int16_t(pEntryKV->GetInt("textureFlags", static_cast<uint16_t>(EPackedTextureFlags::TEXTURE_DEFAULT))),
						pEntryKV->GetBool("useCompression", true),
						pEntryKV->GetBool("deDuplicate", true))
					);
				}
			}
		}
		dir++;
	}
	return vEntryValues;
}

//-----------------------------------------------------------------------------
// Purpose: gets the parts of the directory file name
// Input  : &svDirectoryName - 
//          nCaptureGroup - (1 = locale + target, 2 = level)
// Output : part of directory file name as string
//-----------------------------------------------------------------------------
string CPackedStore::GetNameParts(const string& svDirectoryName, int nCaptureGroup) const
{
	std::smatch smRegexMatches;
	std::regex_search(svDirectoryName, smRegexMatches, DIR_REGEX);

	return smRegexMatches[nCaptureGroup].str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the level name from the directory file name
// Input  : &svDirectoryName - 
// Output : level name as string (e.g. "mp_rr_box")
//-----------------------------------------------------------------------------
string CPackedStore::GetLevelName(const string& svDirectoryName) const
{
	std::smatch smRegexMatches;
	std::regex_search(svDirectoryName, smRegexMatches, DIR_REGEX);

	return smRegexMatches[1].str() + smRegexMatches[2].str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the manifest file associated with the VPK name (must be freed after wards)
// Input  : &svWorkspace - 
//          &svManifestName - 
// Output : KeyValues (build manifest pointer)
//-----------------------------------------------------------------------------
KeyValues* CPackedStore::GetManifest(const string& svWorkspace, const string& svManifestName) const
{
	string svPathOut = Format("%s%s%s.txt", svWorkspace.c_str(), "manifest/", svManifestName.c_str());
	KeyValues* pManifestKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, svPathOut.c_str(), "GAME");

	if (!pManifestKV)
	{
		Warning(eDLL_T::FS, "Failed to parse VPK build manifest: '%s'\n", svPathOut.c_str());
	}

	return pManifestKV;
}

//-----------------------------------------------------------------------------
// Purpose: gets the contents from the global ignore list (.vpkignore)
// Input  : &svWorkspace - 
// Output : a string vector of ignored directories/files and extensions
//-----------------------------------------------------------------------------
vector<string> CPackedStore::GetIgnoreList(const string& svWorkspace) const
{
	string svIgnore = svWorkspace + ".vpkignore";
	FileHandle_t hIgnoreFile = FileSystem()->Open(svIgnore.c_str(), "rt", "GAME");

	if (!hIgnoreFile)
	{
		Warning(eDLL_T::FS, "No ignore file provided; continuing build without...\n");
		return vector<string>();
	}

	vector<string> vIgnore;
	char szIgnore[MAX_PATH];

	while (FileSystem()->ReadLine(szIgnore, sizeof(szIgnore) - 1, hIgnoreFile))
	{
		if (!strstr(szIgnore, "//"))
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

			vIgnore.push_back(szIgnore);
		}
	}

	FileSystem()->Close(hIgnoreFile);
	return vIgnore;
}

//-----------------------------------------------------------------------------
// Purpose: formats the file entry path
// Input  : &svPath - 
//          &svName - 
//          &svExtension - 
// Output : formatted entry path
//-----------------------------------------------------------------------------
string CPackedStore::FormatEntryPath(const string& svPath, const string& svName, const string& svExtension) const
{
	return Format("%s%s%s.%s", svPath.c_str(), svPath.empty() ? "" : "/", svName.c_str(), svExtension.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: builds the VPK manifest file
// Input  : &vBlock - 
//          &svWorkspace - 
//          &svManifestName - 
//-----------------------------------------------------------------------------
void CPackedStore::BuildManifest(const vector<VPKEntryBlock_t>& vBlock, const string& svWorkspace, const string& svManifestName) const
{
	KeyValues kv("BuildManifest");
	KeyValues* pManifestKV = kv.FindKey("BuildManifest", true);

	for (const VPKEntryBlock_t& vEntry : vBlock)
	{
		const VPKChunkDescriptor_t& vDescriptor = vEntry.m_vFragments[0];
		KeyValues* pEntryKV = pManifestKV->FindKey(ConvertToWinPath(vEntry.m_svEntryPath).c_str(), true);

		pEntryKV->SetInt("preloadSize", vEntry.m_iPreloadSize);
		pEntryKV->SetInt("loadFlags", vDescriptor.m_nLoadFlags);
		pEntryKV->SetInt("textureFlags", vDescriptor.m_nTextureFlags);
		pEntryKV->SetBool("useCompression", vDescriptor.m_nCompressedSize != vDescriptor.m_nUncompressedSize);
		pEntryKV->SetBool("deDuplicate", true);
	}

	string svPathOut = Format("%s%s%s.txt", svWorkspace.c_str(), "manifest/", svManifestName.c_str());
	CUtlBuffer uBuf(0i64, 0, CUtlBuffer::TEXT_BUFFER);

	kv.RecursiveSaveToFile(uBuf, 0);

	FileSystem()->CreateDirHierarchy(Format("%s%s", svWorkspace.c_str(), "manifest/").c_str(), "GAME");
	FileSystem()->WriteFile(svPathOut.c_str(), "GAME", uBuf);
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed CRC32 hash
// Input  : &svAssetPath - 
//        : nFileCRC - 
//-----------------------------------------------------------------------------
void CPackedStore::ValidateCRC32PostDecomp(const string& svAssetPath, const uint32_t nFileCRC)
{
	FileHandle_t hAsset = FileSystem()->Open(svAssetPath.c_str(), "rb", "GAME");
	if (!hAsset)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, svAssetPath.c_str());
		return;
	}
	uint32_t nLen = FileSystem()->Size(hAsset);
	uint8_t* pBuf = MemAllocSingleton()->Alloc<uint8_t>(nLen);

	FileSystem()->Read(pBuf, nLen, hAsset);
	FileSystem()->Close(hAsset);

	uint32_t nCrc32 = crc32::update(NULL, pBuf, nLen);
	MemAllocSingleton()->Free(pBuf);

	if (nCrc32 != nFileCRC)
	{
		Warning(eDLL_T::FS, "Computed checksum '0x%lX' doesn't match expected checksum '0x%lX'. File may be corrupt!\n", nCrc32, nFileCRC);
		nCrc32          = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: attempts to deduplicate a chunk of data by comparing it to existing chunks
// Input  : *pEntryBuffer - 
//          &descriptor - 
//          chunkIndex
// Output : true if the chunk was deduplicated, false otherwise
//-----------------------------------------------------------------------------
bool CPackedStore::Deduplicate(const uint8_t* pEntryBuffer, VPKChunkDescriptor_t& descriptor, const size_t chunkIndex)
{
	string entryHash(reinterpret_cast<const char*>(pEntryBuffer), descriptor.m_nUncompressedSize);
	entryHash = sha1(entryHash);

	auto p = m_mChunkHashMap.insert({ entryHash, descriptor });
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
// Purpose: packs all files from workspace path into VPK file
// Input  : &vPair - 
//          &svWorkspace - 
//          &svBuildPath - 
//          bManifestOnly - 
//-----------------------------------------------------------------------------
void CPackedStore::PackWorkspace(const VPKPair_t& vPair, const string& svWorkspace, const string& svBuildPath, bool bManifestOnly)
{
	const string svPackFilePath(svBuildPath + vPair.m_svPackName);

	FileHandle_t hPackFile = FileSystem()->Open(svPackFilePath.c_str(), "wb", "GAME");
	if (!hPackFile)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, svPackFilePath.c_str());
		return;
	}

	uint8_t* pEntryBuffer = MemAllocSingleton()->Alloc<uint8_t>(ENTRY_MAX_LEN);
	if (!pEntryBuffer)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to allocate memory for entry buffer!\n", __FUNCTION__);
		FileSystem()->Close(hPackFile);
		return;
	}

	vector<VPKKeyValues_t> vEntryValues;
	vector<VPKEntryBlock_t> vEntryBlocks;
	KeyValues* pManifestKV = nullptr;

	if (bManifestOnly)
	{
		pManifestKV = GetManifest(svWorkspace, GetLevelName(vPair.m_svDirectoryName));
		vEntryValues = GetEntryValues(svWorkspace, pManifestKV);

		if (pManifestKV)
		{
			pManifestKV->DeleteThis();
		}
	}
	else // Pack all files in workspace.
	{
		vEntryValues = GetEntryValues(svWorkspace);
	}

	uint64_t nSharedTotal = NULL;
	uint32_t nSharedCount = NULL;

	for (size_t i = 0, ps = vEntryValues.size(); i < ps; i++)
	{
		const VPKKeyValues_t& vEntryValue = vEntryValues[i];
		FileHandle_t hAsset = FileSystem()->Open(vEntryValue.m_svEntryPath.c_str(), "rb", "GAME");
		if (!hAsset)
		{
			Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, vEntryValue.m_svEntryPath.c_str());
			continue;
		}

		const char* szDestPath = (vEntryValue.m_svEntryPath.c_str() + svWorkspace.length());
		uint32_t nLen = FileSystem()->Size(hAsset);
		uint8_t* pBuf = MemAllocSingleton()->Alloc<uint8_t>(nLen);

		FileSystem()->Read(pBuf, nLen, hAsset);
		FileSystem()->Seek(hAsset, 0, FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);

		DevMsg(eDLL_T::FS, "Packing entry '%zu' ('%s')\n", i, szDestPath);
		vEntryBlocks.push_back(VPKEntryBlock_t(pBuf, nLen, FileSystem()->Tell(hPackFile), vEntryValue.m_iPreloadSize, 0, vEntryValue.m_nLoadFlags, vEntryValue.m_nTextureFlags, szDestPath));

		VPKEntryBlock_t& vEntryBlock = vEntryBlocks[i];
		for (size_t j = 0, es = vEntryBlock.m_vFragments.size(); j < es; j++)
		{
			VPKChunkDescriptor_t& vDescriptor = vEntryBlock.m_vFragments[j];

			FileSystem()->Read(pEntryBuffer, int(vDescriptor.m_nCompressedSize), hAsset);
			vDescriptor.m_nPackFileOffset = FileSystem()->Tell(hPackFile);

			if (vEntryValue.m_bDeduplicate && Deduplicate(pEntryBuffer, vDescriptor, j))
			{
				nSharedTotal += vDescriptor.m_nCompressedSize;
				nSharedCount++;

				// Data was deduplicated.
				continue;
			}

			if (vEntryValue.m_bUseCompression)
			{
				lzham_compress_status_t lzCompStatus = lzham_compress_memory(&m_lzCompParams, pEntryBuffer, &vDescriptor.m_nCompressedSize, pEntryBuffer,
					vDescriptor.m_nUncompressedSize, nullptr);

				if (lzCompStatus != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
				{
					Warning(eDLL_T::FS, "Status '%d' for chunk '%zu' within entry '%zu' in block '%hu' (chunk packed without compression)\n",
						lzCompStatus, j, i, vEntryBlocks[i].m_iPackFileIndex);

					vDescriptor.m_nCompressedSize = vDescriptor.m_nUncompressedSize;
				}
			}
			else // Write data uncompressed.
			{
				vDescriptor.m_nCompressedSize = vDescriptor.m_nUncompressedSize;
			}

			FileSystem()->Write(pEntryBuffer, int(vDescriptor.m_nCompressedSize), hPackFile);
		}

		MemAllocSingleton()->Free(pBuf);
		FileSystem()->Close(hAsset);
	}

	DevMsg(eDLL_T::FS, "*** Build block totaling '%zu' bytes with '%zu' shared bytes among '%lu' chunks\n", FileSystem()->Tell(hPackFile), nSharedTotal, nSharedCount);
	FileSystem()->Close(hPackFile);

	m_mChunkHashMap.clear();
	MemAllocSingleton()->Free(pEntryBuffer);

	VPKDir_t vDirectory;
	vDirectory.BuildDirectoryFile(svBuildPath + vPair.m_svDirectoryName, vEntryBlocks);
}

//-----------------------------------------------------------------------------
// Purpose: rebuilds manifest and extracts all files from specified VPK file
// Input  : &vDirectory - 
//          &svWorkspace - 
//-----------------------------------------------------------------------------
void CPackedStore::UnpackWorkspace(const VPKDir_t& vDirectory, const string& svWorkspace)
{
	if (vDirectory.m_vHeader.m_nHeaderMarker != VPK_HEADER_MARKER ||
		vDirectory.m_vHeader.m_nMajorVersion != VPK_MAJOR_VERSION ||
		vDirectory.m_vHeader.m_nMinorVersion != VPK_MINOR_VERSION)
	{
		Error(eDLL_T::FS, NO_ERROR, "Unsupported VPK directory file (invalid header criteria)\n");
		return;
	}

	uint8_t* pDestBuffer = MemAllocSingleton()->Alloc<uint8_t>(ENTRY_MAX_LEN);
	uint8_t* pSourceBuffer = MemAllocSingleton()->Alloc<uint8_t>(ENTRY_MAX_LEN);
	if (!pDestBuffer || !pSourceBuffer)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to allocate memory for entry buffer!\n", __FUNCTION__);
		return;
	}

	BuildManifest(vDirectory.m_vEntryBlocks, svWorkspace, GetLevelName(vDirectory.m_svDirectoryPath));
	const string svPath = RemoveFileName(vDirectory.m_svDirectoryPath) + '/';

	for (size_t i = 0, fs = vDirectory.m_vPackFile.size(); i < fs; i++)
	{
		const string svPackFile = svPath + vDirectory.m_vPackFile[i];

		// Read from each pack file.
		FileHandle_t hPackFile = FileSystem()->Open(svPackFile.c_str(), "rb", "GAME");
		if (!hPackFile)
		{
			Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, svPackFile.c_str());
			continue;
		}

		for (size_t j = 0, es = vDirectory.m_vEntryBlocks.size(); j < es; j++)
		{
			const VPKEntryBlock_t& vEntryBlock = vDirectory.m_vEntryBlocks[j];
			if (vEntryBlock.m_iPackFileIndex != static_cast<uint16_t>(i))
			{
				// Chunk doesn't belongs to this block.
				continue;
			}

			string svFilePath;
			CreateDirectories(svWorkspace + vEntryBlock.m_svEntryPath, &svFilePath);
			FileHandle_t hAsset = FileSystem()->Open(svFilePath.c_str(), "wb", "GAME");

			if (!hAsset)
			{
				Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, svFilePath.c_str());
				continue;
			}

			DevMsg(eDLL_T::FS, "Unpacking entry '%zu' from block '%zu' ('%s')\n", j, i, vEntryBlock.m_svEntryPath.c_str());
			for (size_t k = 0, cs = vEntryBlock.m_vFragments.size(); k < cs; k++)
			{
				const VPKChunkDescriptor_t& vChunk = vEntryBlock.m_vFragments[k];
				m_nChunkCount++;

				FileSystem()->Seek(hPackFile, int(vChunk.m_nPackFileOffset), FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);
				FileSystem()->Read(pSourceBuffer, int(vChunk.m_nCompressedSize), hPackFile);

				if (vChunk.m_nCompressedSize == vChunk.m_nUncompressedSize) // Data is not compressed.
				{
					FileSystem()->Write(pSourceBuffer, int(vChunk.m_nUncompressedSize), hAsset);
					break;
				}

				size_t nDstLen = ENTRY_MAX_LEN;
				assert(vChunk.m_nCompressedSize <= nDstLen);

				if (vChunk.m_nCompressedSize > nDstLen)
					break; // Corrupt or invalid chunk descriptor.

				lzham_decompress_status_t lzDecompStatus = lzham_decompress_memory(&m_lzDecompParams, pDestBuffer,
					&nDstLen, pSourceBuffer, vChunk.m_nCompressedSize, nullptr);

				if (lzDecompStatus != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
				{
					Error(eDLL_T::FS, NO_ERROR, "Status '%d' for chunk '%zu' within entry '%zu' in block '%hu' (chunk not decompressed)\n",
						lzDecompStatus, m_nChunkCount, i, vEntryBlock.m_iPackFileIndex);
				}
				else // If successfully decompressed, write to file.
				{
					FileSystem()->Write(pDestBuffer, int(nDstLen), hAsset);
				}
			}

			FileSystem()->Close(hAsset);
			if (m_nChunkCount == vEntryBlock.m_vFragments.size()) // Only validate after last entry in block had been written.
			{
				m_nChunkCount = NULL;
				ValidateCRC32PostDecomp(svFilePath, vEntryBlock.m_nFileCRC);
			}
		}
		FileSystem()->Close(hPackFile);
	}

	MemAllocSingleton()->Free(pDestBuffer);
	MemAllocSingleton()->Free(pSourceBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKKeyValues_t' memory constructor
// Input  : &svEntryPath - 
//          iPreloadSize - 
//          nLoadFlags - 
//          nTextureFlags - 
//          bUseCompression - 
//          bDeduplicate - 
//-----------------------------------------------------------------------------
VPKKeyValues_t::VPKKeyValues_t(const string& svEntryPath, uint16_t iPreloadSize, uint32_t nLoadFlags, uint16_t nTextureFlags, bool bUseCompression, bool bDeduplicate)
{
	m_svEntryPath = svEntryPath;
	m_iPreloadSize = iPreloadSize;
	m_nLoadFlags = nLoadFlags;
	m_nTextureFlags = nTextureFlags;
	m_bUseCompression = bUseCompression;
	m_bDeduplicate = bDeduplicate;
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' file constructor
// Input  : hDirectoryFile - 
//          &svEntryPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(FileHandle_t hDirectoryFile, const string& svEntryPath)
{
	m_svEntryPath = svEntryPath; // Set the entry path.
	StringReplace(m_svEntryPath, "\\", "/"); // Flip windows-style backslash to forward slash.
	StringReplace(m_svEntryPath, " /", "");  // Remove space character representing VPK root.

	FileSystem()->Read(&m_nFileCRC, sizeof(uint32_t), hDirectoryFile);       //
	FileSystem()->Read(&m_iPreloadSize, sizeof(uint16_t), hDirectoryFile);   //
	FileSystem()->Read(&m_iPackFileIndex, sizeof(uint16_t), hDirectoryFile); //

	uint16_t nMarker = 0;
	do // Loop through all chunks in the entry and add to list.
	{
		VPKChunkDescriptor_t entry(hDirectoryFile);
		m_vFragments.push_back(entry);

		FileSystem()->Read(&nMarker, sizeof(nMarker), hDirectoryFile);

	} while (nMarker != static_cast<uint16_t>(PACKFILEINDEX_END));
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' memory constructor
// Input  : *pData - 
//          nLen
//          nOffset - 
//          iPreloadSize - 
//          iPackFileIndex - 
//          nLoadFlags - 
//          nTextureFlags - 
//          &svEntryPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(const uint8_t* pData, size_t nLen, int64_t nOffset, uint16_t iPreloadSize, 
	uint16_t iPackFileIndex, uint32_t nLoadFlags, uint16_t nTextureFlags, const string& svEntryPath)
{
	m_nFileCRC = crc32::update(NULL, pData, nLen);
	m_iPreloadSize = iPreloadSize;
	m_iPackFileIndex = iPackFileIndex;
	m_svEntryPath = svEntryPath;

	size_t nFragmentCount = (nLen + ENTRY_MAX_LEN - 1) / ENTRY_MAX_LEN;
	size_t nFileSize = nLen;
	int64_t nCurrentOffset = nOffset;

	for (size_t i = 0; i < nFragmentCount; i++) // Fragment data into 1 MiB chunks.
	{
		size_t nSize = std::min<uint64_t>(ENTRY_MAX_LEN, nFileSize);
		nFileSize -= nSize;
		m_vFragments.push_back(VPKChunkDescriptor_t(nLoadFlags, nTextureFlags, nCurrentOffset, nSize, nSize));
		nCurrentOffset += nSize;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' file constructor
// Input  : hDirectoryFile - 
//-----------------------------------------------------------------------------
VPKChunkDescriptor_t::VPKChunkDescriptor_t(FileHandle_t hDirectoryFile)
{
	FileSystem()->Read(&m_nLoadFlags, sizeof(uint32_t), hDirectoryFile);        //
	FileSystem()->Read(&m_nTextureFlags, sizeof(uint16_t), hDirectoryFile);     //
	FileSystem()->Read(&m_nPackFileOffset, sizeof(uint64_t), hDirectoryFile);   //
	FileSystem()->Read(&m_nCompressedSize, sizeof(uint64_t), hDirectoryFile);   //
	FileSystem()->Read(&m_nUncompressedSize, sizeof(uint64_t), hDirectoryFile); //
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' memory constructor
// Input  : nLoadFlags - 
//          nTextureFlags - 
//          nArchiveOffset - 
//          nCompressedSize - 
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
// Input  : svLanguage - 
//          svTarget - 
//          &svLevel - 
//          nPatch - 
// Output : a vpk file pair (block and directory file names)
//-----------------------------------------------------------------------------
VPKPair_t::VPKPair_t(string svLanguage, string svTarget, const string& svLevel, int nPatch)
{
	if (std::find(DIR_LOCALE.begin(), DIR_LOCALE.end(), svLanguage) == DIR_LOCALE.end())
	{
		svLanguage = DIR_LOCALE[0];
	}
	if (std::find(DIR_TARGET.begin(), DIR_TARGET.end(), svTarget) == DIR_TARGET.end())
	{
		svTarget = DIR_TARGET[0];
	}

	m_svPackName = Format("%s_%s.bsp.pak000_%03d.vpk", svTarget.c_str(), svLevel.c_str(), nPatch);
	m_svDirectoryName = Format("%s%s_%s.bsp.pak000_dir.vpk", svLanguage.c_str(), svTarget.c_str(), svLevel.c_str());
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &svPath - 
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const string& svDirectoryPath)
{
	Init(svDirectoryPath);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor with sanitation
// Input  : svDirectoryName - 
//          bSanitizeName - retrieve the directory file name from block name
// Output : VPKDir_t
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const string& svDirectoryPath, bool bSanitizeName)
{
	if (!bSanitizeName)
	{
		Init(svDirectoryPath);
		return;
	}

	std::smatch smRegexMatches;
	std::regex_search(svDirectoryPath, smRegexMatches, BLOCK_REGEX);

	if (smRegexMatches.empty())
	{
		Init(svDirectoryPath);
		return;
	}

	string svSanitizedName = svDirectoryPath;
	StringReplace(svSanitizedName, smRegexMatches[0], "pak000_dir");

	bool bHasLocale = false;
	for (const string& svLocale : DIR_LOCALE)
	{
		if (svSanitizedName.find(svLocale) != string::npos)
		{
			bHasLocale = true;
			break;
		}
	}

	if (!bHasLocale) // Only sanitize if no locale was provided.
	{
		string svPackDirPrefix;
		svPackDirPrefix.append(DIR_LOCALE[0]);

		for (const string& svTarget : DIR_TARGET)
		{
			if (svSanitizedName.find(svTarget) != string::npos)
			{
				svPackDirPrefix.append(svTarget);
				StringReplace(svSanitizedName, svTarget, svPackDirPrefix);

				break;
			}
		}
	}

	Init(svSanitizedName);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &svDirectoryPath - 
//-----------------------------------------------------------------------------
void VPKDir_t::Init(const string& svDirectoryPath)
{
	// Create stream to read from each pack file.
	FileHandle_t hDirectory = FileSystem()->Open(svDirectoryPath.c_str(), "rb", "GAME");
	if (!hDirectory)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to open '%s' (insufficient rights?)\n", __FUNCTION__, svDirectoryPath.c_str());
		return;
	}

	FileSystem()->Read(&m_vHeader.m_nHeaderMarker, sizeof(uint32_t), hDirectory);
	FileSystem()->Read(&m_vHeader.m_nMajorVersion, sizeof(uint16_t), hDirectory);  //
	FileSystem()->Read(&m_vHeader.m_nMinorVersion, sizeof(uint16_t), hDirectory);  //
	FileSystem()->Read(&m_vHeader.m_nDirectorySize, sizeof(uint32_t), hDirectory); //
	FileSystem()->Read(&m_vHeader.m_nSignatureSize, sizeof(uint32_t), hDirectory); //

	m_vEntryBlocks = g_pPackedStore->GetEntryBlocks(hDirectory);
	m_svDirectoryPath = svDirectoryPath; // Set path to vpk directory file.

	m_nPackFileCount = 0;
	for (VPKEntryBlock_t vEntry : m_vEntryBlocks)
	{
		if (vEntry.m_iPackFileIndex > m_nPackFileCount)
		{
			m_nPackFileCount = vEntry.m_iPackFileIndex;
		}
	}

	for (uint16_t i = 0; i < m_nPackFileCount + 1; i++)
	{
		string svPackPath = GetPackFile(svDirectoryPath, i);
		m_vPackFile.push_back(svPackPath);
	}

	FileSystem()->Close(hDirectory);
}

//-----------------------------------------------------------------------------
// Purpose: formats pack file path for specific directory file
// Input  : &svDirectoryPath - 
//          iPackFileIndex - 
// output : string
//-----------------------------------------------------------------------------
string VPKDir_t::GetPackFile(const string& svDirectoryPath, uint16_t iPackFileIndex) const
{
	string svPackChunkName = StripLocalePrefix(svDirectoryPath);
	string svPackChunkIndex = Format("pak000_%03d", iPackFileIndex);

	StringReplace(svPackChunkName, "pak000_dir", svPackChunkIndex);
	return svPackChunkName;
}

//-----------------------------------------------------------------------------
// Purpose: strips locale prefix from file path
// Input  : &svDirectoryPath - 
// Output : directory filename without locale prefix
//-----------------------------------------------------------------------------
string VPKDir_t::StripLocalePrefix(const string& svDirectoryPath) const
{
	string svFileName = GetFileName(svDirectoryPath);

	for (const string& svLocale : DIR_LOCALE)
	{
		if (svFileName.find(svLocale) != string::npos)
		{
			StringReplace(svFileName, svLocale, "");
			break;
		}
	}
	return svFileName;
}

//-----------------------------------------------------------------------------
// Purpose: writes the vpk directory header
// Input  : hDirectoryFile - 
//-----------------------------------------------------------------------------
void VPKDir_t::WriteHeader(FileHandle_t hDirectoryFile) const
{
	FileSystem()->Write(&m_vHeader.m_nHeaderMarker, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&m_vHeader.m_nMajorVersion, sizeof(uint16_t), hDirectoryFile);
	FileSystem()->Write(&m_vHeader.m_nMinorVersion, sizeof(uint16_t), hDirectoryFile);
	FileSystem()->Write(&m_vHeader.m_nDirectorySize, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&m_vHeader.m_nSignatureSize, sizeof(uint32_t), hDirectoryFile);
}

//-----------------------------------------------------------------------------
// Purpose: writes the directory tree size
// Input  : hDirectoryFile - 
//-----------------------------------------------------------------------------
void VPKDir_t::WriteTreeSize(FileHandle_t hDirectoryFile) const
{
	FileSystem()->Seek(hDirectoryFile, offsetof(VPKDir_t, m_vHeader.m_nDirectorySize), FileSystemSeek_t::FILESYSTEM_SEEK_HEAD);
	FileSystem()->Write(&m_vHeader.m_nDirectorySize, sizeof(uint32_t), hDirectoryFile);
	FileSystem()->Write(&PACKFILEINDEX_SEP, sizeof(uint32_t), hDirectoryFile);
}

//-----------------------------------------------------------------------------
// Purpose: writes the vpk chunk descriptors
// Input  : hDirectoryFile - 
//			&vMap - 
// Output : number of descriptors written
//-----------------------------------------------------------------------------
uint64_t VPKDir_t::WriteDescriptor(FileHandle_t hDirectoryFile, std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>& vMap) const
{
	uint64_t nDescriptors = NULL;

	for (auto& iKeyValue : vMap)
	{
		FileSystem()->Write(iKeyValue.first.c_str(), int(iKeyValue.first.length() + 1), hDirectoryFile);
		for (auto& jKeyValue : iKeyValue.second)
		{
			FileSystem()->Write(jKeyValue.first.c_str(), int(jKeyValue.first.length() + 1), hDirectoryFile);
			for (auto& vEntry : jKeyValue.second)
			{
				string pszEntryPath = GetFileName(vEntry.m_svEntryPath, true);
				FileSystem()->Write(pszEntryPath.c_str(), int(pszEntryPath.length() + 1), hDirectoryFile);

				FileSystem()->Write(&vEntry.m_nFileCRC, sizeof(uint32_t), hDirectoryFile);
				FileSystem()->Write(&vEntry.m_iPreloadSize, sizeof(uint16_t), hDirectoryFile);
				FileSystem()->Write(&vEntry.m_iPackFileIndex, sizeof(uint16_t), hDirectoryFile);

				for (size_t i = 0, nc = vEntry.m_vFragments.size(); i < nc; i++)
				{
					/*Write chunk descriptor*/
					const VPKChunkDescriptor_t* pDescriptor = &vEntry.m_vFragments[i];

					FileSystem()->Write(&pDescriptor->m_nLoadFlags, sizeof(uint32_t), hDirectoryFile);
					FileSystem()->Write(&pDescriptor->m_nTextureFlags, sizeof(uint16_t), hDirectoryFile);
					FileSystem()->Write(&pDescriptor->m_nPackFileOffset, sizeof(uint64_t), hDirectoryFile);
					FileSystem()->Write(&pDescriptor->m_nCompressedSize, sizeof(uint64_t), hDirectoryFile);
					FileSystem()->Write(&pDescriptor->m_nUncompressedSize, sizeof(uint64_t), hDirectoryFile);

					if (i != (nc - 1))
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
//          &vMap - 
//-----------------------------------------------------------------------------
void VPKDir_t::BuildDirectoryTree(const vector<VPKEntryBlock_t>& vEntryBlocks, std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>& vMap) const
{
	for (const VPKEntryBlock_t& vBlock : vEntryBlocks)
	{
		string svExtension = GetExtension(vBlock.m_svEntryPath);
		string svFilePath = RemoveFileName(vBlock.m_svEntryPath);

		if (svFilePath.empty())
		{
			svFilePath = ' '; // Has to be padded with a space character if empty [root].
		}
		if (!vMap.count(svExtension))
		{
			vMap.insert({ svExtension, std::map<string, std::list<VPKEntryBlock_t>>() });
		}
		if (!vMap[svExtension].count(svFilePath))
		{
			vMap[svExtension].insert({ svFilePath, std::list<VPKEntryBlock_t>() });
		}
		vMap[svExtension][svFilePath].push_back(vBlock);
	}
}

//-----------------------------------------------------------------------------
// Purpose: builds the vpk directory file
// Input  : &svDirectoryPath - 
//          &vEntryBlocks - 
//-----------------------------------------------------------------------------
void VPKDir_t::BuildDirectoryFile(const string& svDirectoryPath, const vector<VPKEntryBlock_t>& vEntryBlocks)
{
	FileHandle_t hDirectoryFile = FileSystem()->Open(svDirectoryPath.c_str(), "wb", "GAME");
	if (!hDirectoryFile)
	{
		Error(eDLL_T::FS, NO_ERROR, "%s - Unable to write to '%s' (read-only?)\n", __FUNCTION__, svDirectoryPath.c_str());
		return;
	}

	auto vMap = std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>();
	BuildDirectoryTree(vEntryBlocks, vMap);

	WriteHeader(hDirectoryFile);
	uint64_t nDescriptors = WriteDescriptor(hDirectoryFile, vMap);

	m_vHeader.m_nDirectorySize = static_cast<uint32_t>(FileSystem()->Tell(hDirectoryFile) - sizeof(VPKDirHeader_t));
	WriteTreeSize(hDirectoryFile);

	FileSystem()->Close(hDirectoryFile);
	DevMsg(eDLL_T::FS, "*** Build directory totaling '%zu' bytes with '%zu' entries and '%zu' descriptors\n",
		size_t(sizeof(VPKDirHeader_t) + m_vHeader.m_nDirectorySize), vEntryBlocks.size(), nDescriptors);
}
//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CPackedStore* g_pPackedStore = new CPackedStore();
