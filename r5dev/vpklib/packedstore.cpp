/*******************************************************************
* ██████╗  ██╗    ██╗   ██╗██████╗ ██╗  ██╗    ██╗     ██╗██████╗  *
* ██╔══██╗███║    ██║   ██║██╔══██╗██║ ██╔╝    ██║     ██║██╔══██╗ *
* ██████╔╝╚██║    ██║   ██║██████╔╝█████╔╝     ██║     ██║██████╔╝ *
* ██╔══██╗ ██║    ╚██╗ ██╔╝██╔═══╝ ██╔═██╗     ██║     ██║██╔══██╗ *
* ██║  ██║ ██║     ╚████╔╝ ██║     ██║  ██╗    ███████╗██║██████╔╝ *
* ╚═╝  ╚═╝ ╚═╝      ╚═══╝  ╚═╝     ╚═╝  ╚═╝    ╚══════╝╚═╝╚═════╝  *
*******************************************************************/
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
	m_lzCompParams.m_dict_size_log2     = VPK_DICT_SIZE;
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
	m_lzDecompParams.m_dict_size_log2   = VPK_DICT_SIZE;
	m_lzDecompParams.m_decompress_flags = lzham_decompress_flags::LZHAM_DECOMP_FLAG_OUTPUT_UNBUFFERED | lzham_decompress_flags::LZHAM_DECOMP_FLAG_COMPUTE_CRC32;
	m_lzDecompParams.m_struct_size      = sizeof(lzham_decompress_params);
}

//-----------------------------------------------------------------------------
// Purpose: gets a directory structure for sepcified file
// Input  : svPackDirFile - 
// Output : VPKDir_t
//-----------------------------------------------------------------------------
VPKDir_t CPackedStore::GetDirectoryFile(string svPackDirFile) const
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
			if (svPackDirFile.find(DIR_CONTEXT[i]) != string::npos)
			{
				for (size_t j = 0; j < DIR_CONTEXT.size(); j++)
				{
					if (svPackDirFile.find(DIR_CONTEXT[j]) != string::npos)
					{
						string svPackDirPrefix = DIR_LOCALE[i] + DIR_LOCALE[i];
						StringReplace(svPackDirFile, DIR_LOCALE[i], svPackDirPrefix);
						goto escape;
					}
				}
			}
		}escape:;
	}

	VPKDir_t vDir(svPackDirFile);
	return vDir;
}

//-----------------------------------------------------------------------------
// Purpose: formats pack file path for specific directory file
// Input  : &svPackDirFile - 
//			iArchiveIndex - 
// output : string
//-----------------------------------------------------------------------------
string CPackedStore::GetPackFile(const string& svPackDirFile, uint16_t iArchiveIndex) const
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
				string svFilePath = FormatEntryPath(svPath, svName, svExtension);
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
	vector<string> vIgnore = GetIgnoreList(svPathIn);

	fs::recursive_directory_iterator dir(svPathIn), end;
	while (dir != end)
	{
		vector<string>::iterator it = std::find(vIgnore.begin(), vIgnore.end(),
			GetExtension(dir->path().filename().u8string(), true, true));
		if (it != vIgnore.end())
		{
			dir.disable_recursion_pending(); // Skip all ignored folders and extensions.
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
	vector<string> vIgnore = GetIgnoreList(svPathIn);

	fs::recursive_directory_iterator dir(svPathIn), end;
	while (dir != end)
	{
		vector<string>::iterator it = std::find(vIgnore.begin(), vIgnore.end(), 
			GetExtension(dir->path().filename().u8string(), true, true));
		if (it != vIgnore.end())
		{
			dir.disable_recursion_pending(); // Skip all ignored folders and extensions.
		}
		else if (!GetExtension(dir->path().u8string()).empty())
		{
			if (!jManifest.is_null())
			{
				try
				{
					string svEntryPath = ConvertToUnixPath(dir->path().u8string());
					if (jManifest.contains(StringReplaceC(svEntryPath, svPathIn, "")))
					{
						vPaths.push_back(svEntryPath);
					}
				}
				catch (const std::exception& ex)
				{
					Warning(eDLL_T::FS, "Exception while reading VPK control file: '%s'\n", ex.what());
				}
			}
		}
		dir++;
	}
	return vPaths;
}

//-----------------------------------------------------------------------------
// Purpose: gets the parts of the directory file name (1 = locale + context, 2 = levelname)
// Input  : &svDirectoryName - 
//          nCaptureGroup - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::GetNameParts(const string& svDirectoryName, int nCaptureGroup) const
{
	std::regex rgArchiveRegex{ R"((?:.*\/)?([^_]*_)(.*)(.bsp.pak000_dir).*)" };
	std::smatch smRegexMatches;
	std::regex_search(svDirectoryName, smRegexMatches, rgArchiveRegex);

	return smRegexMatches[nCaptureGroup].str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the source of the directory file name
// Input  : &svDirectoryName - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::GetSourceName(const string& svDirectoryName) const
{
	std::regex rgArchiveRegex{ R"((?:.*\/)?([^_]*_)(.*)(.bsp.pak000_dir).*)" };
	std::smatch smRegexMatches;
	std::regex_search(svDirectoryName, smRegexMatches, rgArchiveRegex);

	return smRegexMatches[1].str() + smRegexMatches[2].str();
}

//-----------------------------------------------------------------------------
// Purpose: gets the manifest file assosiated with the VPK name
// Input  : &svWorkSpace - 
//          &svManifestName - 
// Output : json
//-----------------------------------------------------------------------------
nlohmann::json CPackedStore::GetManifest(const string& svWorkSpace, const string& svManifestName) const
{
	ostringstream ostream;
	ostream << svWorkSpace << "manifest/" << svManifestName << ".json";
	fs::path fsPath = fs::current_path() /= ostream.str();
	nlohmann::json jsOut;

	if (fs::exists(fsPath))
	{
		try
		{
			ifstream iManifest(fsPath.u8string(), std::ios::binary);
			jsOut = nlohmann::json::parse(iManifest);

			return jsOut;
		}
		catch (const std::exception& ex)
		{
			Warning(eDLL_T::FS, "Exception while parsing VPK control file: '%s'\n", ex.what());
			return jsOut;
		}
	}
	return jsOut;
}

//-----------------------------------------------------------------------------
// Purpose: gets the contents from the global ignore list (.vpkignore)
// Input  : &svWorkSpace - 
// Output : vector<string>
//-----------------------------------------------------------------------------
vector<string> CPackedStore::GetIgnoreList(const string& svWorkSpace) const
{
	fs::path fsIgnore = svWorkSpace + ".vpkignore";
	ifstream iStream(fsIgnore);

	vector<string> vIgnore;
	if (iStream)
	{
		string svIgnore;
		while (std::getline(iStream, svIgnore))
		{
			string::size_type nPos = svIgnore.find("//");
			if (nPos == string::npos)
			{
				if (!svIgnore.empty() && 
					std::find(vIgnore.begin(), vIgnore.end(), svIgnore) == vIgnore.end())
				{
					vIgnore.push_back(svIgnore);
				}
			}
		}
	}
	return vIgnore;
}

//-----------------------------------------------------------------------------
// Purpose: formats the file entry path
// Input  : svPath - 
//          &svName - 
//          &svExtension - 
// Output : string
//-----------------------------------------------------------------------------
string CPackedStore::FormatEntryPath(string svPath, const string& svName, const string& svExtension) const
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
		if (svFileName.find(DIR_LOCALE[i]) != string::npos)
		{
			StringReplace(svFileName, DIR_LOCALE[i], "");
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
//          &svWorkSpace - 
//          &svManifestName - 
//-----------------------------------------------------------------------------
void CPackedStore::BuildManifest(const vector<VPKEntryBlock_t>& vBlock, const string& svWorkSpace, const string& svManifestName) const
{
	nlohmann::json jEntry;

	for (size_t i = 0; i < vBlock.size(); i++)
	{
		jEntry[vBlock[i].m_svEntryPath] =
		{
			{ "preloadSize", vBlock[i].m_iPreloadSize },
			{ "loadFlags", vBlock[i].m_vChunks[0].m_nLoadFlags },
			{ "textureFlags", vBlock[i].m_vChunks[0].m_nTextureFlags },
			{ "useCompression", vBlock[i].m_vChunks[0].m_nCompressedSize != vBlock[i].m_vChunks[0].m_nUncompressedSize },
			{ "useDataSharing", true }
		};
	}

	string svPathOut = svWorkSpace + "manifest/";
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
		Warning(eDLL_T::FS, "Computed checksum '0x%lX' doesn't match expected checksum '0x%lX'. File may be corrupt!\n", m_nAdler32, m_nAdler32_Internal);
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
		Warning(eDLL_T::FS, "Computed checksum '0x%lX' doesn't match expected checksum '0x%lX'. File may be corrupt!\n", m_nCrc32, m_nCrc32_Internal);
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

	vector<string> vPaths;
	vector<VPKEntryBlock_t> vEntryBlocks;
	nlohmann::json jManifest = GetManifest(svPathIn, GetSourceName(vPair.m_svDirectoryName));

	GetIgnoreList(svPathIn);

	if (bManifestOnly)
	{
		vPaths = GetEntryPaths(svPathIn, jManifest);
	}
	else // Pack all files in workspace.
	{
		vPaths = GetEntryPaths(svPathIn);
	}

	uint64_t nSharedTotal = 0i64;
	uint32_t nSharedCount = 0i32;

	for (size_t i = 0; i < vPaths.size(); i++)
	{
		CIOStream reader(vPaths[i], CIOStream::Mode_t::READ);
		if (reader.IsReadable())
		{
			string svDestPath = StringReplaceC(vPaths[i], svPathIn, "");
			uint16_t iPreloadSize  = 0i16;
			uint32_t nLoadFlags    = static_cast<uint32_t>(EPackedLoadFlags::LOAD_VISIBLE) | static_cast<uint32_t>(EPackedLoadFlags::LOAD_CACHE);
			uint16_t nTextureFlags = static_cast<uint16_t>(EPackedTextureFlags::TEXTURE_DEFAULT); // !TODO: Reverse these.
			bool bUseCompression   = true;
			bool bUseDataSharing   = true;

			if (!jManifest.is_null())
			{
				try
				{
					nlohmann::json jEntry = jManifest[svDestPath];
					if (!jEntry.is_null())
					{
						iPreloadSize    = jEntry.at("preloadSize").get<uint32_t>();
						nLoadFlags      = jEntry.at("loadFlags").get<uint32_t>();
						nTextureFlags   = jEntry.at("textureFlags").get<uint16_t>();
						bUseCompression = jEntry.at("useCompression").get<bool>();
						bUseDataSharing = jEntry.at("useDataSharing").get<bool>();
					}
				}
				catch (const std::exception& ex)
				{
					Warning(eDLL_T::FS, "Exception while reading VPK control file: '%s'\n", ex.what());
				}
			}
			DevMsg(eDLL_T::FS, "Packing entry '%zu' ('%s')\n", i, svDestPath.c_str());

			vEntryBlocks.push_back(VPKEntryBlock_t(reader.GetVector(), writer.GetPosition(), iPreloadSize, 0, nLoadFlags, nTextureFlags, svDestPath));
			for (size_t j = 0; j < vEntryBlocks[i].m_vChunks.size(); j++)
			{
				uint8_t* pSrc  = new uint8_t[vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize];
				uint8_t* pDest = new uint8_t[vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize];

				bool bShared = false;
				bool bCompressed = bUseCompression;

				reader.Read(*pSrc, vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize);
				vEntryBlocks[i].m_vChunks[j].m_nArchiveOffset = writer.GetPosition();

				if (bUseCompression)
				{
					m_lzCompStatus = lzham_compress_memory(&m_lzCompParams, pDest, 
						&vEntryBlocks[i].m_vChunks[j].m_nCompressedSize, pSrc, 
						vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);
					if (m_lzCompStatus != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
					{
						Warning(eDLL_T::FS, "Status '%d' for chunk '%zu' within entry '%zu' in block '%hu' (chunk packed without compression)\n", 
							m_lzCompStatus, j, i, vEntryBlocks[i].m_iPackFileIndex);

						vEntryBlocks[i].m_vChunks[j].m_nCompressedSize = vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize;
						memmove(pDest, pSrc, vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize);
					}
				}
				else // Write data uncompressed.
				{
					vEntryBlocks[i].m_vChunks[j].m_nCompressedSize = vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize;
					memmove(pDest, pSrc, vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize);
				}
				vEntryBlocks[i].m_vChunks[j].m_bIsCompressed = vEntryBlocks[i].m_vChunks[j].m_nCompressedSize != vEntryBlocks[i].m_vChunks[j].m_nUncompressedSize;

				if (bUseDataSharing)
				{
					string svEntryHash = sha1(string(reinterpret_cast<char*>(pDest), vEntryBlocks[i].m_vChunks[j].m_nCompressedSize));

					if (auto it{ m_mChunkHashMap.find(svEntryHash) }; it != std::end(m_mChunkHashMap))
					{
						DevMsg(eDLL_T::FS, "Mapping chunk '%zu' ('%s') to existing chunk at '0x%llx'\n", j, svEntryHash.c_str(), it->second.m_nArchiveOffset);

						vEntryBlocks[i].m_vChunks[j].m_nArchiveOffset = it->second.m_nArchiveOffset;
						nSharedTotal += it->second.m_nCompressedSize;
						nSharedCount++;
						bShared = true;
					}
					else // Add entry to hashmap.
					{
						m_mChunkHashMap.insert({ svEntryHash, vEntryBlocks[i].m_vChunks[j] });
						bShared = false;
					}
				}
				if (!bShared)
				{
					writer.Write(pDest, vEntryBlocks[i].m_vChunks[j].m_nCompressedSize);
				}

				delete[] pDest;
				delete[] pSrc;
			}
		}
	}
	DevMsg(eDLL_T::FS, "*** Build block totalling '%zu' bytes with '%zu' shared bytes among '%lu' chunks\n", writer.GetPosition(), nSharedTotal, nSharedCount);
	m_mChunkHashMap.clear();

	VPKDir_t vDir = VPKDir_t();
	vDir.Build(svPathOut + vPair.m_svDirectoryName, vEntryBlocks);
}

//-----------------------------------------------------------------------------
// Purpose: extracts all files from specified VPK file
// Input  : &vDir - 
//          &svPathOut - 
//-----------------------------------------------------------------------------
void CPackedStore::UnpackAll(const VPKDir_t& vDir, const string& svPathOut)
{
	if (vDir.m_vHeader.m_nHeaderMarker != VPK_HEADER_MARKER ||
		vDir.m_vHeader.m_nMajorVersion != VPK_MAJOR_VERSION ||
		vDir.m_vHeader.m_nMinorVersion != VPK_MINOR_VERSION)
	{
		Error(eDLL_T::FS, "Unsupported VPK directory file (invalid header criteria)\n");
		return;
	}
	BuildManifest(vDir.m_vEntryBlocks, svPathOut, GetSourceName(vDir.m_svDirPath));

	for (size_t i = 0; i < vDir.m_vPackFile.size(); i++)
	{
		fs::path fspVpkPath(vDir.m_svDirPath);
		string svPath = fspVpkPath.parent_path().u8string() + '\\' + vDir.m_vPackFile[i];
		CIOStream iStream(svPath, CIOStream::Mode_t::READ); // Create stream to read from each archive.

		for ( size_t j = 0; j < vDir.m_vEntryBlocks.size(); j++)
		{
			if (vDir.m_vEntryBlocks[j].m_iPackFileIndex != static_cast<uint16_t>(i))
			{
				goto escape;
			}
			else // Chunk belongs to this block.
			{
				string svFilePath = CreateDirectories(svPathOut + vDir.m_vEntryBlocks[j].m_svEntryPath);
				CIOStream oStream(svFilePath, CIOStream::Mode_t::WRITE);

				if (!oStream.IsWritable())
				{
					Error(eDLL_T::FS, "Unable to write file '%s'\n", svFilePath.c_str());
					continue;
				}
				DevMsg(eDLL_T::FS, "Unpacking entry '%zu' from block '%zu' ('%s')\n", j, i, vDir.m_vEntryBlocks[j].m_svEntryPath.c_str());

				for (VPKChunkDescriptor_t vChunk : vDir.m_vEntryBlocks[j].m_vChunks)
				{
					m_nChunkCount++;

					uint8_t* pCompressedData = new uint8_t[vChunk.m_nCompressedSize];

					iStream.SetPosition(vChunk.m_nArchiveOffset);
					iStream.Read(*pCompressedData, vChunk.m_nCompressedSize);

					if (vChunk.m_bIsCompressed)
					{
						uint8_t* pLzOutputBuf = new uint8_t[vChunk.m_nUncompressedSize];
						m_lzDecompStatus = lzham_decompress_memory(&m_lzDecompParams, pLzOutputBuf, 
							&vChunk.m_nUncompressedSize, pCompressedData, 
							vChunk.m_nCompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);

						if (m_lzDecompStatus != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
						{
							Error(eDLL_T::FS, "Status '%d' for chunk '%zu' within entry '%zu' in block '%hu' (chunk not decompressed)\n", 
								m_lzDecompStatus, m_nChunkCount, i, vDir.m_vEntryBlocks[j].m_iPackFileIndex);
						}
						else // If successfully decompressed, write to file.
						{
							oStream.Write(pLzOutputBuf, vChunk.m_nUncompressedSize);
						}
						delete[] pLzOutputBuf;
					}
					else // If not compressed, write raw data into output file.
					{
						oStream.Write(pCompressedData, vChunk.m_nUncompressedSize);
					}
					delete[] pCompressedData;
				}

				if (m_nChunkCount == vDir.m_vEntryBlocks[j].m_vChunks.size()) // Only validate after last entry in block had been written.
				{
					m_nChunkCount = 0;
					m_nCrc32_Internal = vDir.m_vEntryBlocks[j].m_nFileCRC;

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
//          svEntryPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(CIOStream* pReader, string svEntryPath)
{
	StringReplace(svEntryPath, "\\", "/"); // Flip windows-style backslash to forward slash.
	StringReplace(svEntryPath, " /", "" ); // Remove space character representing VPK root.

	this->m_svEntryPath = svEntryPath; // Set the entry path.
	pReader->Read<uint32_t>(this->m_nFileCRC);       //
	pReader->Read<uint16_t>(this->m_iPreloadSize);   //
	pReader->Read<uint16_t>(this->m_iPackFileIndex); //

	do // Loop through all chunks in the entry and push them to the vector.
	{
		VPKChunkDescriptor_t entry(pReader);
		this->m_vChunks.push_back(entry);
	} while (pReader->Read<uint16_t>() != UINT16_MAX);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKEntryBlock_t' memory constructor
// Input  : &vData - 
//          nOffset - 
//          nPreloadSize - 
//          nArchiveIndex - 
//          nLoadFlags - 
//          nTextureFlags - 
//          &svBlockPath - 
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(const vector<uint8_t> &vData, int64_t nOffset, uint16_t nPreloadSize, uint16_t nArchiveIndex, uint32_t nLoadFlags, uint16_t nTextureFlags, const string& svEntryPath)
{
	m_nFileCRC = crc32::update(m_nFileCRC, vData.data(), vData.size());
	m_iPreloadSize = nPreloadSize;
	m_iPackFileIndex = nArchiveIndex;
	m_svEntryPath = svEntryPath;

	size_t nEntryCount = (vData.size() + ENTRY_MAX_LEN - 1) / ENTRY_MAX_LEN;
	size_t nDataSize = vData.size();
	int64_t nCurrentOffset = nOffset;

	for (size_t i = 0; i < nEntryCount; i++) // Fragment data into 1 MiB chunks
	{
		size_t nSize = std::min<uint64_t>(ENTRY_MAX_LEN, nDataSize);
		nDataSize -= nSize;
		m_vChunks.push_back(VPKChunkDescriptor_t(nLoadFlags, nTextureFlags, nCurrentOffset, nSize, nSize));
		nCurrentOffset += nSize;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' file constructor
// Input  : *pReader - 
//-----------------------------------------------------------------------------
VPKChunkDescriptor_t::VPKChunkDescriptor_t(CIOStream* pReader)
{
	pReader->Read<uint32_t>(this->m_nLoadFlags);        //
	pReader->Read<uint16_t>(this->m_nTextureFlags);     //
	pReader->Read<uint64_t>(this->m_nArchiveOffset);    //
	pReader->Read<uint64_t>(this->m_nCompressedSize);   //
	pReader->Read<uint64_t>(this->m_nUncompressedSize); //
	this->m_bIsCompressed = (this->m_nCompressedSize != this->m_nUncompressedSize);
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKChunkDescriptor_t' memory constructor
// Input  : nLoadFlags - 
//          nTextureFlags - 
//          nArchiveOffset - 
//          nCompressedSize - 
//          nUncompressedSize - 
//-----------------------------------------------------------------------------
VPKChunkDescriptor_t::VPKChunkDescriptor_t(uint32_t nLoadFlags, uint16_t nTextureFlags, uint64_t nArchiveOffset, uint64_t nCompressedSize, uint64_t nUncompressedSize)
{
	m_nLoadFlags = nLoadFlags;
	m_nTextureFlags = nTextureFlags;
	m_nArchiveOffset = nArchiveOffset;

	m_nCompressedSize = nCompressedSize;
	m_nUncompressedSize = nUncompressedSize;
}

//-----------------------------------------------------------------------------
// Purpose: 'VPKDir_t' file constructor
// Input  : &szPath - 
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const string& svPath)
{
	CIOStream reader(svPath, CIOStream::Mode_t::READ);

	reader.Read<uint32_t>(this->m_vHeader.m_nHeaderMarker);
	reader.Read<uint16_t>(this->m_vHeader.m_nMajorVersion);  //
	reader.Read<uint16_t>(this->m_vHeader.m_nMinorVersion);  //
	reader.Read<uint32_t>(this->m_vHeader.m_nDirectorySize); //
	reader.Read<uint32_t>(this->m_nFileDataSize);            //

	this->m_vEntryBlocks = g_pPackedStore->GetEntryBlocks(&reader);
	this->m_svDirPath = svPath; // Set path to vpk directory file.

	for (VPKEntryBlock_t vEntry : this->m_vEntryBlocks)
	{
		if (vEntry.m_iPackFileIndex > this->m_iPackFileCount)
		{
			this->m_iPackFileCount = vEntry.m_iPackFileIndex;
		}
	}

	for (uint16_t i = 0; i < this->m_iPackFileCount + 1; i++)
	{
		string svArchivePath = g_pPackedStore->GetPackFile(svPath, i);
		this->m_vPackFile.push_back(svArchivePath);
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
	uint64_t nDescriptors = 0i64;

	writer.Write<uint32_t>(this->m_vHeader.m_nHeaderMarker);
	writer.Write<uint16_t>(this->m_vHeader.m_nMajorVersion);
	writer.Write<uint16_t>(this->m_vHeader.m_nMinorVersion);
	writer.Write<uint32_t>(this->m_vHeader.m_nDirectorySize);
	writer.Write<uint32_t>(this->m_vHeader.m_nSignatureSize);

	for (VPKEntryBlock_t vBlock : vEntryBlocks)
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

	for (auto& iKeyValue : vMap)
	{
		writer.WriteString(iKeyValue.first);
		for (auto& jKeyValue : iKeyValue.second)
		{
			writer.WriteString(jKeyValue.first);
			for (auto& vEntry : jKeyValue.second)
			{
				writer.WriteString(GetFileName(vEntry.m_svEntryPath, true));
				{/*Write entry block*/
					writer.Write(vEntry.m_nFileCRC);
					writer.Write(vEntry.m_iPreloadSize);
					writer.Write(vEntry.m_iPackFileIndex);

					for (size_t i = 0; i < vEntry.m_vChunks.size(); i++)
					{
						{/*Write chunk descriptor*/
							writer.Write(vEntry.m_vChunks[i].m_nLoadFlags);
							writer.Write(vEntry.m_vChunks[i].m_nTextureFlags);
							writer.Write(vEntry.m_vChunks[i].m_nArchiveOffset);
							writer.Write(vEntry.m_vChunks[i].m_nCompressedSize);
							writer.Write(vEntry.m_vChunks[i].m_nUncompressedSize);
						}

						if (i != (vEntry.m_vChunks.size() - 1))
						{
							const ushort s = 0;
							writer.Write(s);
						}
						else
						{
							const ushort s = UINT16_MAX;
							writer.Write(s);
						}
						nDescriptors++;
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

	DevMsg(eDLL_T::FS, "*** Build directory totalling '%zu' bytes with '%zu' entries and '%zu' descriptors\n", 
		size_t(sizeof(VPKDirHeader_t) + m_vHeader.m_nDirectorySize), vEntryBlocks.size(), nDescriptors);
}
///////////////////////////////////////////////////////////////////////////////
CPackedStore* g_pPackedStore = new CPackedStore();
