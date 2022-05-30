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
// Purpose: obtains archive chunk path for specific file
//-----------------------------------------------------------------------------
string CPackedStore::GetPackChunkFile(string svPackDirFile, int iArchiveIndex)
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
// Purpose: returns populated pack dir struct for specified pack dir file
//-----------------------------------------------------------------------------
VPKDir_t CPackedStore::GetPackDirFile(string svPackDirFile)
{
	/*| PACKDIRFILE |||||||||||||||||||||||||||||||||||||||||||||||||||||||||*/
	std::regex rgArchiveRegex("pak000_([0-9]{3})");
	std::smatch smRegexMatches;

	std::regex_search(svPackDirFile, smRegexMatches, rgArchiveRegex);

	if (smRegexMatches.size() != 0)
	{
		StringReplace(svPackDirFile, smRegexMatches[0], "pak000_dir");

		for (int i = 0; i < LANGUAGE_PACKS; i++)
		{
			if (strstr(svPackDirFile.c_str(), DIR_LIBRARY_PREFIX[i].c_str()))
			{
				for (int j = 0; j < LIBRARY_PACKS; j++)
				{
					if (strstr(svPackDirFile.c_str(), DIR_LIBRARY_PREFIX[j].c_str()))
					{
						string svPackDirPrefix = DIR_LOCALE_PREFIX[i] + DIR_LOCALE_PREFIX[i];
						StringReplace(svPackDirFile, DIR_LOCALE_PREFIX[i].c_str(), svPackDirPrefix.c_str());
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
// Purpose: obtains and returns the entry block to the vector
//-----------------------------------------------------------------------------
vector<VPKEntryBlock_t> CPackedStore::GetEntryBlocks(CIOStream* pReader)
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
				string svFilePath = FormatBlockPath(svName, svPath, svExtension);
				vBlocks.push_back(VPKEntryBlock_t(pReader, svFilePath));
			}
		}
	}
	return vBlocks;
}

//-----------------------------------------------------------------------------
// Purpose: formats the entry block path
//-----------------------------------------------------------------------------
string CPackedStore::FormatBlockPath(string svName, string svPath, string svExtension)
{
	if (!svPath.empty())
	{
		svPath += "\\";
	}
	return svPath + svName + "." + svExtension;
}

//-----------------------------------------------------------------------------
// Purpose: strips locale prefix from file path
//-----------------------------------------------------------------------------
string CPackedStore::StripLocalePrefix(string svPackDirFile)
{
	fs::path fspPackDirFile(svPackDirFile);
	string svFileName = fspPackDirFile.filename().u8string();

	for (int i = 0; i < LANGUAGE_PACKS; i++)
	{
		if (strstr(svFileName.c_str(), DIR_LOCALE_PREFIX[i].c_str()))
		{
			StringReplace(svFileName, DIR_LOCALE_PREFIX[i].c_str(), "");
			break;
		}
	}
	return svFileName;
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed ADLER32 hash
//-----------------------------------------------------------------------------
void CPackedStore::ValidateAdler32PostDecomp(string svAssetFile)
{
	uint32_t adler_init = {};
	ifstream istream(svAssetFile, fstream::binary);

	istream.seekg(0, fstream::end);
	m_vHashBuffer.resize(istream.tellg());
	istream.seekg(0, fstream::beg);
	istream.read((char*)m_vHashBuffer.data(), m_vHashBuffer.size());

	m_nAdler32 = adler32::update(adler_init, m_vHashBuffer.data(), m_vHashBuffer.size());

	if (m_nAdler32 != m_nAdler32_Internal)
	{
		Warning(eDLL_T::FS, "Warning: ADLER32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svAssetFile.c_str(), m_nAdler32, m_nAdler32_Internal);
		m_nAdler32          = 0;
		m_nAdler32_Internal = 0;
	}

	istream.close();
	m_vHashBuffer.clear();
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed CRC32 hash
//-----------------------------------------------------------------------------
void CPackedStore::ValidateCRC32PostDecomp(string svDirAsset)
{
	uint32_t crc32_init = {};
	ifstream istream(svDirAsset, fstream::binary);

	istream.seekg(0, fstream::end);
	m_vHashBuffer.resize(istream.tellg());
	istream.seekg(0, fstream::beg);
	istream.read((char*)m_vHashBuffer.data(), m_vHashBuffer.size());

	m_nCrc32 = crc32::update(crc32_init, m_vHashBuffer.data(), m_vHashBuffer.size());

	if (m_nCrc32 != m_nCrc32_Internal)
	{
		Warning(eDLL_T::FS, "Warning: CRC32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svDirAsset.c_str(), m_nCrc32, m_nCrc32_Internal);
		m_nCrc32          = 0;
		m_nCrc32_Internal = 0;
	}

	istream.close();
	m_vHashBuffer.clear();
}

void CPackedStore::PackAll(string svDirIn, string svPathOut)
{
	vector<uint8_t> uData; // Raw file data
	vector<VPKEntryBlock_t> vEntryBlocks;

	ifstream iData(svDirIn, fstream::binary);

	iData.seekg(0, fstream::end);
	uData.resize(iData.tellg());
	iData.seekg(0, fstream::beg);
	iData.read(reinterpret_cast<char*>(uData.data()), uData.size());

	ofstream oBlock(svDirIn, fstream::binary);
	vEntryBlocks.push_back(VPKEntryBlock_t(uData, oBlock.tellp(), 0, 0x101, 0, svDirIn));

	for (size_t i = 0; i < vEntryBlocks.size(); i++)
	{
		for (VPKEntryDescriptor_t& entry : vEntryBlocks.at(i).m_vvEntries)
		{
			uint8_t* pSrc = new uint8_t[entry.m_nUncompressedSize];
			uint8_t* pDest = new uint8_t[entry.m_nUncompressedSize];

			iData.seekg(entry.m_nArchiveOffset);       // Seek to entry offset in archive.
			iData.read(reinterpret_cast<char*>(pSrc), entry.m_nUncompressedSize); // Read compressed data from archive.

			m_lzCompStatus = lzham_compress_memory(&m_lzCompParams, pDest, &entry.m_nCompressedSize, pSrc, entry.m_nUncompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);

			if (m_lzCompStatus != lzham_compress_status_t::LZHAM_COMP_STATUS_SUCCESS)
			{
				Error(eDLL_T::FS, "Error: failed compression for an entry within block '%s' for archive '%d'\n", vEntryBlocks.at(i).m_svBlockPath.c_str(), i);
				Error(eDLL_T::FS, "'lzham::lzham_lib_compress_memory' returned with status '%d'.\n", m_lzCompStatus);
			}

			entry.m_nArchiveOffset = oBlock.tellp();
			entry.m_bIsCompressed = entry.m_nCompressedSize != entry.m_nUncompressedSize;

			oBlock.write(reinterpret_cast<char*>(pDest), entry.m_nCompressedSize);

			delete[] pSrc;
			delete[] pDest;
		}
	}

	VPKDir_t vDir = VPKDir_t();
	vDir.Build("englishclient_mp_rr_test.bsp.pak000_dir.vpk", vEntryBlocks); // [!!! <<DEVELOPMENT>> !!!]
}

//-----------------------------------------------------------------------------
// Purpose: extracts all files from specified vpk file
//-----------------------------------------------------------------------------
void CPackedStore::UnpackAll(VPKDir_t vpkDir, string svPathOut)
{
	for (int i = 0; i < vpkDir.m_vsvArchives.size(); i++)
	{
		fs::path fspVpkPath(vpkDir.m_svDirPath);
		string svPath = fspVpkPath.parent_path().u8string() + "\\" + vpkDir.m_vsvArchives[i];
		ifstream packChunkStream(svPath, std::ios_base::binary); // Create stream to read from each archive.

		for ( VPKEntryBlock_t block : vpkDir.m_vvEntryBlocks)
		{
			// Escape if block archive index is not part of the extracting archive chunk index.
			if (block.m_iArchiveIndex != i) { goto escape; }
			else
			{
				string svFilePath = CreateDirectories(svPathOut + "\\" + block.m_svBlockPath);
				ofstream outFileStream(svFilePath, std::ios_base::binary | std::ios_base::out);

				if (!outFileStream.is_open())
				{
					Error(eDLL_T::FS, "Error: unable to access file '%s'!\n", svFilePath.c_str());
				}
				outFileStream.clear(); // Make sure file is empty before writing.
				for (VPKEntryDescriptor_t entry : block.m_vvEntries)
				{
					char* pCompressedData = new char[entry.m_nCompressedSize];
					memset(pCompressedData, 0, entry.m_nCompressedSize); // Compressed region.

					packChunkStream.seekg(entry.m_nArchiveOffset);       // Seek to entry offset in archive.
					packChunkStream.read(pCompressedData, entry.m_nCompressedSize); // Read compressed data from archive.

					if (entry.m_bIsCompressed)
					{
						lzham_uint8* pLzOutputBuf = new lzham_uint8[entry.m_nUncompressedSize];
						m_lzDecompStatus = lzham_decompress_memory(&m_lzDecompParams, pLzOutputBuf, 
							(size_t*)&entry.m_nUncompressedSize, (lzham_uint8*)pCompressedData, 
							entry.m_nCompressedSize, &m_nAdler32_Internal, &m_nCrc32_Internal);

						if (fs_packedstore_entryblock_stats->GetBool())
						{
							DevMsg(eDLL_T::FS, "--------------------------------------------------------------\n");
							DevMsg(eDLL_T::FS, "] Block path            : '%s'\n", block.m_svBlockPath.c_str());
							DevMsg(eDLL_T::FS, "] Entry count           : '%llu'\n", block.m_vvEntries.size());
							DevMsg(eDLL_T::FS, "] Compressed size       : '%llu'\n", entry.m_nCompressedSize);
							DevMsg(eDLL_T::FS, "] Uncompressed size     : '%llu'\n", entry.m_nUncompressedSize);
							DevMsg(eDLL_T::FS, "] Static CRC32 hash     : '0x%lX'\n", block.m_nCrc32);
							DevMsg(eDLL_T::FS, "] Computed CRC32 hash   : '0x%lX'\n", m_nCrc32_Internal);
							DevMsg(eDLL_T::FS, "] Computed ADLER32 hash : '0x%lX'\n", m_nAdler32_Internal);
							DevMsg(eDLL_T::FS, "--------------------------------------------------------------\n");
						}

						if (block.m_vvEntries.size() == 1) // Internal checksum can only match block checksum if entry size is 1.
						{
							if (block.m_nCrc32 != m_nCrc32_Internal)
							{
								Warning(eDLL_T::FS, "Warning: CRC32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", block.m_svBlockPath.c_str(), m_nCrc32_Internal, block.m_nCrc32);
							}
						}
						else { m_nEntryCount++; }

						if (m_lzDecompStatus != lzham_decompress_status_t::LZHAM_DECOMP_STATUS_SUCCESS)
						{
							Error(eDLL_T::FS, "Error: failed decompression for an entry within block '%s' in archive '%d'!\n", block.m_svBlockPath.c_str(), i);
							Error(eDLL_T::FS, "'lzham::lzham_lib_decompress_memory' returned with status '%d'.\n", m_lzDecompStatus);
						}
						else
						{
							// If successfully decompressed, write to file.
							outFileStream.write((char*)pLzOutputBuf, entry.m_nUncompressedSize);
						}
						delete[] pLzOutputBuf;
					}
					else
					{
						// If not compressed, write raw data into output file.
						outFileStream.write(pCompressedData, entry.m_nUncompressedSize);
					}
					delete[] pCompressedData;
				}
				outFileStream.close();
				if (m_nEntryCount == block.m_vvEntries.size()) // Only validate after last entry in block had been written.
				{
					// Set internal hash to precomputed entry hash for post decompress validation.
					m_nCrc32_Internal = block.m_nCrc32;

					ValidateCRC32PostDecomp(svFilePath);
					//ValidateAdler32PostDecomp(svFilePath);
					m_nEntryCount = 0;
				}
			}escape:;
		}
		packChunkStream.close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 'vpk_entry_block' constructor
//-----------------------------------------------------------------------------
VPKEntryBlock_t::VPKEntryBlock_t(CIOStream* reader, string svPath)
{
	std::replace(svPath.begin(), svPath.end(), '/', '\\'); // Flip forward slashes in filepath to windows-style backslash.

	this->m_svBlockPath = svPath; // Set path of block.
	reader->Read<uint32_t>(this->m_nCrc32);        //
	reader->Read<uint16_t>(this->m_nPreloadBytes); //
	reader->Read<uint16_t>(this->m_iArchiveIndex); //

	do // Loop through all entries in the block and push them to the vector.
	{
		VPKEntryDescriptor_t entry(reader);
		this->m_vvEntries.push_back(entry);
	} while (reader->Read<uint16_t>() != 0xFFFF);
}

VPKEntryBlock_t::VPKEntryBlock_t(const vector<uint8_t> &vData, int64_t nOffset, uint16_t nArchiveIndex, uint32_t nEntryFlags, uint16_t nTextureFlags, string svBlockPath)
{
	m_nCrc32 = crc32::update(NULL, vData.data(), vData.size());
	m_nPreloadBytes = 0;
	m_iArchiveIndex = nArchiveIndex;
	m_svBlockPath = svBlockPath;

	int nEntryCount = (vData.size() + RVPK_MAX_BLOCK - 1) / RVPK_MAX_BLOCK;
	m_vvEntries = vector<VPKEntryDescriptor_t>(nEntryCount);

	int64_t nDataSize = vData.size();
	int64_t nCurrentOffset = nOffset;
	for (int i = 0; i < nEntryCount; i++)
	{
		int64_t nSize = min(RVPK_MAX_BLOCK, nDataSize);
		nDataSize -= nSize;
		m_vvEntries[i] = VPKEntryDescriptor_t(nEntryFlags, nTextureFlags, nCurrentOffset, nSize, nSize);
		nCurrentOffset += nSize;
	}
}

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
//-----------------------------------------------------------------------------
VPKDir_t::VPKDir_t(const string& svPath)
{
	CIOStream reader;

	reader.Open(svPath, eStreamFileMode::READ);
	reader.Read<uint32_t>(this->m_vHeader.m_nFileMagic);

	if (this->m_vHeader.m_nFileMagic != RVPK_DIR_MAGIC)
	{
		Error(eDLL_T::FS, "Error: vpk_dir file '%s' has invalid magic!\n", svPath.c_str());
		//return;
	}

	reader.Read<uint16_t>(this->m_vHeader.m_nMajorVersion); //
	reader.Read<uint16_t>(this->m_vHeader.m_nMinorVersion); //
	reader.Read<uint32_t>(this->m_vHeader.m_nTreeSize);     //
	reader.Read<uint32_t>(this->m_nFileDataSize);           //

	DevMsg(eDLL_T::FS, "______________________________________________________________\n");
	DevMsg(eDLL_T::FS, "] HEADER_DETAILS ---------------------------------------------\n");
	DevMsg(eDLL_T::FS, "] File Magic     : '%lu'\n", this->m_vHeader.m_nFileMagic);
	DevMsg(eDLL_T::FS, "] Major Version  : '%hu'\n", this->m_vHeader.m_nMajorVersion);
	DevMsg(eDLL_T::FS, "] Minor Version  : '%hu'\n", this->m_vHeader.m_nMinorVersion);
	DevMsg(eDLL_T::FS, "] Tree Size      : '%lu'\n", this->m_vHeader.m_nTreeSize);
	DevMsg(eDLL_T::FS, "] File Data Size : '%lu'\n", this->m_nFileDataSize);

	this->m_vvEntryBlocks = g_pPackedStore->GetEntryBlocks(&reader);
	this->m_svDirPath = svPath; // Set path to vpk_dir file.

	for (VPKEntryBlock_t block : this->m_vvEntryBlocks)
	{
		if (block.m_iArchiveIndex > this->m_iArchiveCount)
		{
			this->m_iArchiveCount = block.m_iArchiveIndex;
		}
	}

	DevMsg(eDLL_T::FS, "______________________________________________________________\n");
	DevMsg(eDLL_T::FS, "] PACK_CHUNKS ------------------------------------------------\n");

	for (int i = 0; i < this->m_iArchiveCount + 1; i++)
	{
		string svArchivePath = g_pPackedStore->GetPackChunkFile(svPath, i);
		DevMsg(eDLL_T::FS, "] '%s'\n", svArchivePath.c_str());
		this->m_vsvArchives.push_back(svArchivePath);
	}
}

//-----------------------------------------------------------------------------
// Purpose: builds the VPKDir file
// Input  : &svFileName - 
//          &vEntryBlocks - 
//-----------------------------------------------------------------------------
void VPKDir_t::Build(const string& svFileName, const vector<VPKEntryBlock_t>& vEntryBlocks)
{
	CIOStream writer(svFileName, eStreamFileMode::WRITE);
	auto vMap = std::map<string, std::map<string, std::list<VPKEntryBlock_t>>>();

	writer.Write<uint32_t>(this->m_vHeader.m_nFileMagic);
	writer.Write<uint16_t>(this->m_vHeader.m_nMajorVersion);
	writer.Write<uint16_t>(this->m_vHeader.m_nMinorVersion);
	writer.Write<uint32_t>(this->m_vHeader.m_nTreeSize);
	writer.Write<uint32_t>(this->m_vHeader.m_nTreeSize);

	for (VPKEntryBlock_t vBlock : vEntryBlocks)
	{
		string svExtension = GetExtension(vBlock.m_svBlockPath);
		string svFileName = GetFileName(vBlock.m_svBlockPath);
		string svFilePath = RemoveFileName(vBlock.m_svBlockPath);

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
					writer.Write(eKeyValue.m_nPreloadBytes);
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
				writer.Write<uint8_t>('\0');
			}
			writer.Write<uint8_t>('\0');
		}
		writer.Write<uint8_t>('\0');
	}
	m_vHeader.m_nTreeSize = static_cast<uint32_t>(writer.GetPosition() - sizeof(VPKHeader_t));

	writer.SetPosition(offsetof(VPKDir_t, m_vHeader.m_nTreeSize));
	writer.Write(this->m_vHeader.m_nTreeSize);
	writer.Write(0);
}
///////////////////////////////////////////////////////////////////////////////
CPackedStore* g_pPackedStore = new CPackedStore();
