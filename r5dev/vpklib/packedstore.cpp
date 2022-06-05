/***********************************************************************
* ██████╗ ██████╗     ██╗   ██╗██████╗ ██╗  ██╗    ██╗     ██╗██████╗  *
* ██╔══██╗╚════██╗    ██║   ██║██╔══██╗██║ ██╔╝    ██║     ██║██╔══██╗ *
* ██████╔╝ █████╔╝    ██║   ██║██████╔╝█████╔╝     ██║     ██║██████╔╝ *
* ██╔══██╗██╔═══╝     ╚██╗ ██╔╝██╔═══╝ ██╔═██╗     ██║     ██║██╔══██╗ *
* ██║  ██║███████╗     ╚████╔╝ ██║     ██║  ██╗    ███████╗██║██████╔╝ *
* ╚═╝  ╚═╝╚══════╝      ╚═══╝  ╚═╝     ╚═╝  ╚═╝    ╚══════╝╚═╝╚═════╝  *
***********************************************************************/

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "mathlib/adler32.h"
#include "mathlib/crc32.h"
#include "mathlib/sha1.h"
#include "vpklib/packedstore.h"

//-----------------------------------------------------------------------------
// Purpose: initialize parameters for compression algorithm
//-----------------------------------------------------------------------------
void CPackedStore::InitLzCompParams(void)
{
	/*| PARAMETERS ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	m_lzCompParams.m_dict_size_log2     = RVPK_DICT_SIZE;
	m_lzCompParams.m_level              = lzham_compress_level::LZHAM_COMP_LEVEL_UBER;
	m_lzCompParams.m_compress_flags     = lzham_compress_flags::LZHAM_COMP_FLAG_DETERMINISTIC_PARSING | lzham_compress_flags::LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO;
	m_lzCompParams.m_max_helper_threads = -1;
}

//-----------------------------------------------------------------------------
// Purpose: initialize parameters for decompression algorithm
//-----------------------------------------------------------------------------
void CPackedStore::InitLzDecompParams(void)
{
	/*| PARAMETERS ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	m_lzDecompParams.m_dict_size_log2   = RVPK_DICT_SIZE;
	m_lzDecompParams.m_decompress_flags = lzham_decompress_flags::LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED | lzham_decompress_flags::LZHAM_DECOMP_FLAG_COMPUTE_CRC32;
	m_lzDecompParams.m_struct_size      = sizeof(lzham_decompress_params);
}

//-----------------------------------------------------------------------------
// Purpose: returns populated pack dir struct for specified pack dir file
// Input  : svPackDirFile - 
// Output : VPKDir_t
//-----------------------------------------------------------------------------
VPKDir_t CPackedStore::GetPackDirFile(string svPackDirFile) const
{
	/*| PACKDIRFILE |||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	std::regex rgArchiveRegex("pak000_([0-9]{3})");
	std::smatch smRegexMatches;

	std::regex_search(svPackDirFile, smRegexMatches, rgArchiveRegex);

	if (smRegexMatches.size() != 0)
	{
		StringReplace(svPackDirFile, smRegexMatches[0], "pak000_dir");

		for (size_t i = 0; i < DIR_LOCALE.size(); i++)
		{
			if (strstr(svPackDirFile.c_str(), DIR_CONTEXT[i].c_str()))
			{
				for (size_t j = 0; j < DIR_CONTEXT.size(); j++)
				{
					if (strstr(svPackDirFile.c_str(), DIR_CONTEXT[j].c_str()))
					{
						string svPackDirPrefix = DIR_LOCALE[i] + DIR_LOCALE[i];
						StringReplace(svPackDirFile, DIR_LOCALE[i].c_str(), svPackDirPrefix.c_str());
						goto escape;
					}
				}
			}
		}escape:;
	}

	VPKDir_t vpk_dir(svPackDirFile);
	return vpk_dir;
}

//-----------------------------------------------------------------------------
// Purpose: obtains archive chunk path for specific file
// Input  : &svPackDirFile - 
//			iArchiveIndex - 
// output : string
//-----------------------------------------------------------------------------
string CPackedStore::GetPackChunkFile(const string& svPackDirFile, int iArchiveIndex) const
{
	/*| ARCHIVES ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	string svPackChunkFile = StripLocalePrefix(svPackDirFile);
	ostringstream oss;

	oss << std::setw(3) << std::setfill('0') << iArchiveIndex;
	string svPackChunkIndex = "pak000_" + oss.str();

	StringReplace(svPackChunkFile, "pak000_dir", svPackChunkIndex);
	return svPackChunkFile;
}

//-----------------------------------------------------------------------------
// Purpose: obtains and returns the entry block to the vector
// Input  : *pReader - 
// output : vector<VPKEntryBlock_t>
//-----------------------------------------------------------------------------
vector<VPKEntryBlock_t> CPackedStore::GetEntryBlocks(CIOStream* pReader) const
{
	/*| ENTRYBLOCKS |||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	string svName, svPath, svExtension;
	vector<VPKEntryBlock_t> vBlocks;
	while (!(svExtension = pReader->ReadString()).empty())
	{
		while (!(svPath = pReader->ReadString()).empty())
		{
			while (!(svName = pReader->ReadString()).empty())
			{
				string svFilePath = FormatBlockPath(svPath, svName, svExtension);
				vBlocks.push_back(VPKEntryBlock_t(pReader, svFilePath));
			}
		}
	}
	return vBlocks;
}

//-----------------------------------------------------------------------------
// Purpose: scans the input directory and returns the paths to the vector
// Input  : &svPathIn - 
// Output : vector<string>
//-----------------------------------------------------------------------------
vector<string> CPackedStore::GetEntryPaths(const string& svPathIn) const
{
	vector<string> vPaths;
	fs::recursive_directory_iterator dir(svPathIn), end;
	while (dir != end)
	{
		if (dir->path().filename() == "manifest")
		{
			dir.disable_recursion_pending(); // Don't recurse into this directory (manifest files only).
		}
		if (!GetExtension(dir->path().u8string()).empty())
		{
			vPaths.push_back(ConvertToUnixPath(dir->path().u8string()));
		}
		dir++;
	}
	return vPaths;
}

//-----------------------------------------------------------------------------
// Purpose: scans the input directory and returns the paths to the vector if path exists in manifest
// Input  : &svPathIn - 
//          &jManifest - 
// Output : vector<string>
//-----------------------------------------------------------------------------
vector<string> CPackedStore::GetEntryPaths(const string& svPathIn, const nlohmann::json& jManifest) const
{
	vector<string> vPaths;
	fs::recursive_directory_iterator dir(svPathIn), end;
	while (dir != end)
	{
		if (dir->path().filename() == "manifest")
		{
			dir.disable_recursion_pending(); // Don't recurse into this directory (manifest files only).
		}
		if (!GetExtension(dir->path().u8string()).empty())
		{
			if (!jManifest.is_null())
			{
				try
				{
					string svBlockPath = ConvertToUnixPath(dir->path().u8string());
					if (jManifest.contains(StringReplaceC(svBlockPath, svPathIn, "")))
					{
						vPaths.push_back(svBlockPath);
					}
				}
				catch (const std::exception& ex)
				{
					Warning(eDLL_T::FS, "Exception while reading VPK manifest file: '%s'\n", ex.what());
				}
			}
		}
		dir++;
	}
	return vPaths;
}

//-----------------------------------------------------------------------------
// Purpose: gets the raw level name from the directory file name
// Input  : &svDirectoryName - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::GetLevelName(const string& svDirectoryName) const
{
	std::regex rgArchiveRegex{ R"([^_]*_(.*)(.bsp.pak000_dir).*)" };
	std::smatch smRegexMatches;
	std::regex_search(svDirectoryName, smRegexMatches, rgArchiveRegex);

	return smRegexMatches[1].str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the manifest file assosiated with the VPK name
// Input  : &svManifestName - 
// Output : json
//-----------------------------------------------------------------------------
nlohmann::json CPackedStore::GetManifest(const string& svWorkSpace, const string& svManifestName) const
{
	ostringstream ostream;
	ostream << svWorkSpace << "manifest/" << svManifestName << ".json";
	fs::path fsPath = fs::current_path() /= ostream.str();

	if (fs::exists(fsPath))
	{
		nlohmann::json jsOut;
		try
		{
			ifstream iManifest(fsPath.string().c_str(), std::ios::binary);
			jsOut = nlohmann::json::parse(iManifest);

			return jsOut;
		}
		catch (const std::exception& ex)
		{
			Warning(eDLL_T::FS, "Exception while parsing VPK manifest file: '%s'\n", ex.what());
			return jsOut;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: formats the entry block path
// Input  : svPath - 
//			&svName - 
//			&svExtension - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::FormatBlockPath(string svPath, const string& svName, const string& svExtension) const
{
	if (!svPath.empty())
	{
		svPath += '/';
	}
	return svPath + svName + '.' + svExtension;
}

//-----------------------------------------------------------------------------
// Purpose: strips locale prefix from file path
// Input  : &svDirectoryFile - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::StripLocalePrefix(const string& svDirectoryFile) const
{
	fs::path fsDirectoryFile(svDirectoryFile);
	string svFileName = fsDirectoryFile.filename().u8string();

	for (size_t i = 0; i < DIR_LOCALE.size(); i++)
	{
		if (strstr(svFileName.c_str(), DIR_LOCALE[i].c_str()))
		{
			StringReplace(svFileName, DIR_LOCALE[i].c_str(), "");
			break;
		}
	}
	return svFileName;
}

//-----------------------------------------------------------------------------
// Purpose: builds a valid file name for the VPK
// Input  : svLanguage - 
//          svContext - 
//          &svPakName - 
//          nPatch - 
// Output : VPKPair_t
//-----------------------------------------------------------------------------
VPKPair_t CPackedStore::BuildFileName(string svLanguage, string svContext, const string& svPakName, int nPatch) const
{
	if (std::find(DIR_LOCALE.begin(), DIR_LOCALE.end(), svLanguage) == DIR_LOCALE.end())
	{
		svLanguage = DIR_LOCALE[0];
	}
	if (std::find(DIR_CONTEXT.begin(), DIR_CONTEXT.end(), svContext) == DIR_CONTEXT.end())
	{
		svContext = DIR_CONTEXT[0];
	}

	VPKPair_t vPair;
	vPair.m_svBlockName = fmt::format("{:s}_{:s}.bsp.pak000_{:03d}{:s}", svContext, svPakName, nPatch, ".vpk");
	vPair.m_svDirectoryName = fmt::format("{:s}{:s}_{:s}.bsp.pak000_{:s}", svLanguage, svContext, svPakName, "dir.vpk");

	return vPair;
}

//-----------------------------------------------------------------------------
// Purpose: builds the VPK manifest file
// Input  : &vBlock - 
//          &svOutPath - 
// Output : VPKPair_t
//-----------------------------------------------------------------------------
void CPackedStore::BuildManifest(const vector<VPKEntryBlock_t>& vBlock, const string& svWorkSpace, const string& svManifestName) const
{
	nlohmann::json jEntry;

	for (size_t i = 0; i < vBlock.size(); i++)
	{
		jEntry[vBlock[i].m_svBlockPath] =
		{
			{ "preloadData", vBlock[i].m_nPreloadData },
			{ "entryFlags", vBlock[i].m_vvEntries[0].m_nEntryFlags },
			{ "textureFlags", vBlock[i].m_vvEntries[0].m_nTextureFlags },
			{ "useCompression", vBlock[i].m_vvEntries[0].m_nCompressedSize != vBlock[i].m_vvEntries[0].m_nUncompressedSize },
			{ "useDataSharing", true }
		};
	}

	string svPathOut = svWorkSpace + "manifest\\";
	fs::create_directories(svPathOut);

	ofstream oManifest(svPathOut + svManifestName + ".json");
	oManifest << jEntry.dump(4);
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed ADLER32 hash
// Input  : &svAssetFile - 
//-----------------------------------------------------------------------------
void CPackedStore::ValidateAdler32PostDecomp(const string& svAssetFile)
{
	CIOStream reader(svAssetFile, CIOStream::Mode_t::READ);
	m_nAdler32 = adler32::update(NULL, reader.GetData(), reader.GetSize());

	if (m_nAdler32 != m_nAdler32_Internal)
	{
		Warning(eDLL_T::FS, "ADLER32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svAssetFile.c_str(), m_nAdler32, m_nAdler32_Internal);
		m_nAdler32          = NULL;
		m_nAdler32_Internal = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed CRC32 hash
// Input  : &svAssetFile - 
//-----------------------------------------------------------------------------
void CPackedStore::ValidateCRC32PostDecomp(const string& svAssetFile)
{
	CIOStream reader(svAssetFile, CIOStream::Mode_t::READ);
	m_nCrc32 = crc32::update(NULL, reader.GetData(), reader.GetSize());

	if (m_nCrc32 != m_nCrc32_Internal)
	{
		Warning(eDLL_T::FS, "CRC32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svAssetFile.c_str(), m_nCrc32, m_nCrc32_Internal);
		m_nCrc32          = NULL;
		m_nCrc32_Internal = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: packs all files from specified path into VPK file
// Input  : &vPair - 
//          &svPathIn - 
//          &svPathOut - 
//          bManifestOnly - 
//-----------------------------------------------------------------------------
void CPackedStore::PackAll(const VPKPair_t& vPair, const string& svPathIn, const string& svPathOut, bool bManifestOnly)
{
	CIOStream writer(svPathOut + vPair.m_svBlockName, CIOStream::Mode_t::WRITE);

	string svLevelName = GetLevelName(vPair.m_svDirectoryName);
	vector<string> vPaths;
	vector<VPKEntryBlock_t> vEntryBlocks;
	nlohmann::json jManifest = GetManifest(svPathIn, svLevelName);

	if (bManifestOnly)
	{
		vPaths = GetEntryPaths(svPathIn, jManifest);
	}
	else // Compress all files in workspace.
	{
		vPaths = GetEntryPaths(svPathIn);
	}

	for (size_t i = 0; i < vPaths.size(); i++)
	{
		CIOStream reader(vPaths[i], CIOStream::Mode_t::READ);
		if (reader.IsReadable())
		{
			uint16_t nPreloadData  = 0i16;
			uint32_t nEntryFlags   = static_cast<uint32_t>(EPackedEntryFlags::ENTRY_VISIBLE) | static_cast<uint32_t>(EPackedEntryFlags::ENTRY_CACHE);
			uint16_t nTextureFlags = static_cast<short>(EPackedTextureFlags::TEXTURE_DEFAULT); // !TODO: Reverse these.
			bool bUseCompression   = true;
			bool bUseDataSharing   = true;

			if (!jManifest.is_null())
			{
				try
				{
					nlohmann::json jEntry = jManifest[StringReplaceC(vPaths[i], svPathIn, "")];
					if (!jEntry.is_null())
					{
						nPreloadData    = jEntry.at("preloadData").get<uint32_t>();
						nEntryFlags     = jEntry.at("entryFlags").get<uint32_t>();
						nTextureFlags   = jEntry.at("textureFlags").get<uint16_t>();
						bUseCompression = jEntry.at("useCompression").get<bool>();
						bUseDataSharing = jEntry.at("useDataSharing").get<bool>();
					}
				}
				catch (const std::exception& ex)
				{
					Warning(eDLL_T::FS, "Exception while reading VPK manifest file: '%s'\n", ex.what());
				}
			}

			vEntryBlocks.push_back(VPKEntryBlock_t(reader.GetVector(), writer.GetPosition(), nPreloadData, 0, nEntryFlags, nTextureFlags, StringReplaceC(vPaths[i], svPathIn, "")));
			for (size_t j = 0; j < vEntryBlocks[i].m_vvEntries.size(); j++)
			{
				uint8_t* pSrc  = new uint8_t[vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize];
				uint8_t* pDest = new uint8_t[vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize];

				bool bShared = false;
				bool bCompressed = bUseCompression;

				reader.Read(*pSrc, vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize);
				vEntryBlocks[i].m_vvEntries[j].m_nArchiveOffset = writer.GetPosition();

				if (bUseCompression)
				{
					m_lzCompStatus = lzham_compress_memory(&m_lzCompParams, pDest, &vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize, pSrc, vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);
					if (m_lzCompStatus != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
					{
						Warning(eDLL_T::FS, "Failed compression for entry '%llu' within block '%s' for chunk '%hu'\n", j, vEntryBlocks[i].m_svBlockPath.c_str(), vEntryBlocks[i].m_iArchiveIndex);
						Warning(eDLL_T::FS, "'lzham::lzham_lib_compress_memory' returned with status '%d' (entry will be packed without compression).\n", m_lzCompStatus);

						vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize = vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize;
						memmove(pDest, pSrc, vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize);
					}
				}
				else // Write data uncompressed.
				{
					vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize = vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize;
					memmove(pDest, pSrc, vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize);
				}
				vEntryBlocks[i].m_vvEntries[j].m_bIsCompressed = vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize != vEntryBlocks[i].m_vvEntries[j].m_nUncompressedSize;

				if (bUseDataSharing)
				{
					string svEntryHash = sha1(string(reinterpret_cast<char*>(pDest), vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize));

					if (auto it{ m_mEntryHashMap.find(svEntryHash) }; it != std::end(m_mEntryHashMap))
					{
						vEntryBlocks[i].m_vvEntries[j] = it->second;
						bShared = true;
					}
					else // Add entry to hashmap.
					{
						m_mEntryHashMap.insert({ svEntryHash, vEntryBlocks[i].m_vvEntries[j] });
						bShared = false;
					}
				}
				if (!bShared)
				{
					writer.Write(pDest, vEntryBlocks[i].m_vvEntries[j].m_nCompressedSize);
				}

				delete[] pDest;
				delete[] pSrc;
			}
		}
	}

	m_mEntryHashMap.clear();
	VPKDir_t vDir = VPKDir_t();
	vDir.Build(svPathOut + vPair.m_svDirectoryName, vEntryBlocks);
}

//-----------------------------------------------------------------------------
// Purpose: extracts all files from specified VPK file
// Input  : &vpkDir - 
//          &svPathOut - 
//-----------------------------------------------------------------------------
void CPackedStore::UnpackAll(const VPKDir_t& vpkDir, const string& svPathOut)
{
	BuildManifest(vpkDir.m_vvEntryBlocks, svPathOut, GetLevelName(vpkDir.m_svDirPath));

	for (size_t i = 0; i < vpkDir.m_vsvArchives.size(); i++)
	{
		fs::path fspVpkPath(vpkDir.m_svDirPath);
		string svPath = fspVpkPath.parent_path().u8string() + '\\' + vpkDir.m_vsvArchives[i];
		CIOStream iStream(svPath, CIOStream::Mode_t::READ); // Create stream to read from each archive.

		for ( VPKEntryBlock_t vBlock : vpkDir.m_vvEntryBlocks)
		{
			if (vBlock.m_iArchiveIndex != static_cast<uint16_t>(i))
			{
				goto escape;
			}
			else // Chunk belongs to this block.
			{
				string svFilePath = CreateDirectories(svPathOut + vBlock.m_svBlockPath, true);
				CIOStream oStream(svFilePath, CIOStream::Mode_t::WRITE);

				if (!oStream.IsWritable())
				{
					Error(eDLL_T::FS, "Unable to write file '%s'\n", svFilePath.c_str());
					continue;
				}

				for (VPKEntryDescriptor_t vEntry : vBlock.m_vvEntries)
				{
					m_nEntryCount++;

					uint8_t* pCompressedData = new uint8_t[vEntry.m_nCompressedSize];

					iStream.SetPosition(vEntry.m_nArchiveOffset);
					iStream.Read(*pCompressedData, vEntry.m_nCompressedSize);

					if (vEntry.m_bIsCompressed)
					{
						uint8_t* pLzOutputBuf = new uint8_t[vEntry.m_nUncompressedSize];
						m_lzDecompStatus = lzham_decompress_memory(&m_lzDecompParams, pLzOutputBuf, 
							&vEntry.m_nUncompressedSize, pCompressedData, 
							vEntry.m_nCompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);

						if (m_lzDecompStatus != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
						{
							Error(eDLL_T::FS, "Failed decompression for entry '%llu' within block '%s' in chunk '%llu'!\n", m_nEntryCount, vBlock.m_svBlockPath.c_str(), i);
							Error(eDLL_T::FS, "'lzham::lzham_lib_decompress_memory' returned with status '%d'.\n", m_lzDecompStatus);
						}
						else // If successfully decompressed, write to file.
						{
							oStream.Write(pLzOutputBuf, vEntry.m_nUncompressedSize);
						}
						delete[] pLzOutputBuf;
					}
					else // If not compressed, write raw data into output file.
					{
						oStream.Write(pCompressedData, vEntry.m_nUncompressedSize);
					}
					delete[] pCompressedData;
				}

				if (m_nEntryCount == vBlock.m_vvEntries.size()) // Only validate after last entry in block had been written.
				{
					m_nEntryCount = 0;
					m_nCrc32_Internal = vBlock.m_nCrc32;

					oStream.Flush();
					ValidateCRC32PostDecomp(svFilePath);
					//ValidateAdler32PostDecomp(svFilePath);
				}
			}escape:;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' file constructor
// Input  : *pReader - 
//          svBlockPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(CIOStream* pReader, string svBlockPath)
{
	std::replace(svBlockPath.begin(), svBlockPath.end(), '\\', '/'); // Flip windows-style backslash to forward slash.

	this->m_svBlockPath = svBlockPath; // Set path of block.
	pReader->Read<uint32_t>(this->m_nCrc32);        //
	pReader->Read<uint16_t>(this->m_nPreloadData);  //
	pReader->Read<uint16_t>(this->m_iArchiveIndex); //

	do // Loop through all entries in the block and push them to the vector.
	{
		VPKEntryDescriptor_t entry(pReader);
		this->m_vvEntries.push_back(entry);
	} while (pReader->Read<uint16_t>() != UINT16_MAX);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' memory constructor
// Input  : &vData - 
//          nOffset - 
//          nPreloadData - 
//          nArchiveIndex - 
//          nEntryFlags - 
//          nTextureFlags - 
//          svBlockPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(const vector<uint8_t> &vData, int64_t nOffset, uint16_t nPreloadData, uint16_t nArchiveIndex, uint32_t nEntryFlags, uint16_t nTextureFlags, const string& svBlockPath)
{
	m_nCrc32 = crc32::update(m_nCrc32, vData.data(), vData.size());
	m_nPreloadData = nPreloadData;
	m_iArchiveIndex = nArchiveIndex;
	m_svBlockPath = svBlockPath;

	int nEntryCount = (vData.size() + ENTRY_MAX - 1) / ENTRY_MAX;
	uint64_t nDataSize = vData.size();
	int64_t nCurrentOffset = nOffset;
	for (int i = 0; i < nEntryCount; i++)
	{
		uint64_t nSize = std::min<uint64_t>(ENTRY_MAX, nDataSize);
		nDataSize -= nSize;
		m_vvEntries.push_back(VPKEntryDescriptor_t(nEntryFlags, nTextureFlags, nCurrentOffset, nSize, nSize));
		nCurrentOffset += nSize;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryDescriptor_t' constructor
// Input  : &nEntryFlags - 
//          &nTextureFlags - 
//          &nArchiveOffset - 
//          &nCompressedSize - 
//          &nUncompressedSize - 
//-----------------------------------------------------------------------------
VPKEntryDescriptor_t::VPKEntryDescriptor_t(uint32_t nEntryFlags, uint16_t nTextureFlags, uint64_t nArchiveOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize)
{
	m_nEntryFlags = nEntryFlags;
	m_nTextureFlags = nTextureFlags;
	m_nArchiveOffset = nArchiveOffset;

	m_nCompressedSize = nCompressedSize;
	m_nUncompressedSize = nUncompressedSize;
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' constructor
// Input  : *pReader - 
//-----------------------------------------------------------------------------
VPKEntryDescriptor_t::VPKEntryDescriptor_t(CIOStream* pReader)
{
	pReader->Read<uint32_t>(this->m_nEntryFlags);       //
	pReader->Read<uint16_t>(this->m_nTextureFlags);     //
	pReader->Read<uint64_t>(this->m_nArchiveOffset);    //
	pReader->Read<uint64_t>(this->m_nCompressedSize);   //
	pReader->Read<uint64_t>(this->m_nUncompressedSize); //
	this->m_bIsCompressed = (this->m_nCompressedSize != this->m_nUncompressedSize);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &szPath - 
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const string& svPath)
{
	CIOStream reader(svPath, CIOStream::Mode_t::READ);
	reader.Read<uint32_t>(this->m_vHeader.m_nHeaderMarker);

	if (this->m_vHeader.m_nHeaderMarker != VPK_HEADER_MARKER)
	{
		Error(eDLL_T::FS, "VPK directory file '%s' has invalid magic!\n", svPath.c_str());
		return;
	}

	reader.Read<uint16_t>(this->m_vHeader.m_nMajorVersion);  //
	reader.Read<uint16_t>(this->m_vHeader.m_nMinorVersion);  //
	reader.Read<uint32_t>(this->m_vHeader.m_nDirectorySize); //
	reader.Read<uint32_t>(this->m_nFileDataSize);            //

	this->m_vvEntryBlocks = g_pPackedStore->GetEntryBlocks(&reader);
	this->m_svDirPath = svPath; // Set path to vpk_dir file.

	for (VPKEntryBlock_t block : this->m_vvEntryBlocks)
	{
		if (block.m_iArchiveIndex > this->m_iArchiveCount)
		{
			this->m_iArchiveCount = block.m_iArchiveIndex;
		}
	}

	for (int i = 0; i < this->m_iArchiveCount + 1; i++)
	{
		string svArchivePath = g_pPackedStore->GetPackChunkFile(svPath, i);
		this->m_vsvArchives.push_back(svArchivePath);
	}
}

//-----------------------------------------------------------------------------
// Purpose: builds the vpk directory file
// Input  : &svDirectoryFile - 
//          &vEntryBlocks - 
//-----------------------------------------------------------------------------
void VPKDir_t::Build(const string& svDirectoryFile, const vector<VPKEntryBlock_t>& vEntryBlocks)
{
	CIOStream writer(svDirectoryFile, CIOStream::Mode_t::WRITE);
	auto vMap = std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>();

	writer.Write<uint32_t>(this->m_vHeader.m_nHeaderMarker);
	writer.Write<uint16_t>(this->m_vHeader.m_nMajorVersion);
	writer.Write<uint16_t>(this->m_vHeader.m_nMinorVersion);
	writer.Write<uint32_t>(this->m_vHeader.m_nDirectorySize);
	writer.Write<uint32_t>(this->m_vHeader.m_nDirectorySize);

	for (VPKEntryBlock_t vBlock : vEntryBlocks)
	{
		string svExtension = GetExtension(vBlock.m_svBlockPath);
		string svFilePath = RemoveFileName(vBlock.m_svBlockPath);

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

	for (auto& iKeyValue : vMap)
	{
		writer.WriteString(iKeyValue.first);
		for (auto& jKeyValue : iKeyValue.second)
		{
			writer.WriteString(jKeyValue.first);
			for (auto& eKeyValue : jKeyValue.second)
			{
				writer.WriteString(GetFileName(eKeyValue.m_svBlockPath, true));
				{/*Write entry block*/
					writer.Write(eKeyValue.m_nCrc32);
					writer.Write(eKeyValue.m_nPreloadData);
					writer.Write(eKeyValue.m_iArchiveIndex);

					for (size_t i = 0; i < eKeyValue.m_vvEntries.size(); i++)
					{
						{/*Write entry descriptor*/
							writer.Write(eKeyValue.m_vvEntries[i].m_nEntryFlags);
							writer.Write(eKeyValue.m_vvEntries[i].m_nTextureFlags);
							writer.Write(eKeyValue.m_vvEntries[i].m_nArchiveOffset);
							writer.Write(eKeyValue.m_vvEntries[i].m_nCompressedSize);
							writer.Write(eKeyValue.m_vvEntries[i].m_nUncompressedSize);
						}

						if (i != (eKeyValue.m_vvEntries.size() - 1))
						{
							const ushort s = 0;
							writer.Write(s);
						}
						else
						{
							const ushort s = UINT16_MAX;
							writer.Write(s);
						}
					}
				}
			}
			writer.Write<uint8_t>('\0');
		}
		writer.Write<uint8_t>('\0');
	}
	writer.Write<uint8_t>('\0');
	m_vHeader.m_nDirectorySize = static_cast<uint32_t>(writer.GetPosition() - sizeof(VPKDirHeader_t));

	writer.SetPosition(offsetof(VPKDir_t, m_vHeader.m_nDirectorySize));
	writer.Write(this->m_vHeader.m_nDirectorySize);
	writer.Write(0);
}
///////////////////////////////////////////////////////////////////////////////
CPackedStore* g_pPackedStore = new CPackedStore();
