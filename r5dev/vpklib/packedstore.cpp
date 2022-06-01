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
	m_lzCompParams.m_level              = lzham_compress_level::LZHAM_COMP_LEVEL_FASTER;
	//m_lzCompParams.m_compress_flags     = lzham_compress_flags::LZHAM_COMP_FLAG_DETERMINISTIC_PARSING | lzham_compress_flags::LZHAM_COMP_FLAG_TRADEOFF_DECOMPRESSION_RATE_FOR_COMP_RATIO;
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
// Purpose: scans the input directory and returns the paths to the vector
//-----------------------------------------------------------------------------
vector<string> CPackedStore::GetEntryPaths(const string& svPathIn) const
{
	vector<string> vPaths;
	for (const fs::directory_entry& dirEntry : fs::recursive_directory_iterator(fs_packedstore_workspace->GetString()))
	{
		if (!GetExtension(dirEntry.path().u8string()).empty())
		{
			vPaths.push_back(ConvertToUnixPath(dirEntry.path().u8string()));
		}
	}
	return vPaths;
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
void CPackedStore::ValidateAdler32PostDecomp(const string& svAssetFile)
{
	CIOStream reader(svAssetFile, CIOStream::Mode_t::READ);
	m_nAdler32 = adler32::update(m_nAdler32, reader.GetData(), reader.GetSize());

	if (m_nAdler32 != m_nAdler32_Internal)
	{
		Warning(eDLL_T::FS, "Warning: ADLER32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svAssetFile.c_str(), m_nAdler32, m_nAdler32_Internal);
		m_nAdler32          = 0;
		m_nAdler32_Internal = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: validates extraction result with precomputed CRC32 hash
//-----------------------------------------------------------------------------
void CPackedStore::ValidateCRC32PostDecomp(const string& svAssetFile)
{
	CIOStream reader(svAssetFile, CIOStream::Mode_t::READ);
	m_nCrc32 = crc32::update(m_nCrc32, reader.GetData(), reader.GetSize());

	if (m_nCrc32 != m_nCrc32_Internal)
	{
		Warning(eDLL_T::FS, "Warning: CRC32 checksum mismatch for entry '%s' computed value '0x%lX' doesn't match expected value '0x%lX'. File may be corrupt!\n", svAssetFile.c_str(), m_nCrc32, m_nCrc32_Internal);
		m_nCrc32          = 0;
		m_nCrc32_Internal = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
CPackedStore* g_pPackedStore = new CPackedStore();
