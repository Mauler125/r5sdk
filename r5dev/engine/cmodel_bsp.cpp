//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/sys_utils.h"
#include "engine/cmodel_bsp.h"
#include "rtech/rtech_game.h"

//-----------------------------------------------------------------------------
// Purpose: loads required pakfile assets for specified BSP
// Input  : svSetFile - 
//-----------------------------------------------------------------------------
void MOD_PreloadPak(const std::string& svSetFile)
{
	std::ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << g_pHostState->m_levelName << ".json";

	std::filesystem::path fsPath = std::filesystem::current_path() /= ostream.str();
	if (FileExists(fsPath.string().c_str()))
	{
		nlohmann::json jsIn;
		try
		{
			std::ifstream iPakLoadDefFile(fsPath, std::ios::binary); // Parse prerequisites file.
			iPakLoadDefFile >> jsIn;
			iPakLoadDefFile.close();

			if (!jsIn.is_null())
			{
				if (!jsIn["rpak"].is_null())
				{
					for (auto it = jsIn["rpak"].begin(); it != jsIn["rpak"].end(); ++it)
					{
						if (it.value().is_string())
						{
							std::string svToLoad = it.value().get<std::string>() + ".rpak";
							std::uint32_t nPakId = RTech_AsyncLoad((void*)svToLoad.c_str(), g_pMallocPool.GetPtr(), 4, 0);

							if (nPakId == -1)
							{
								Error(eDLL_T::RTECH, "RTech_AsyncLoad: failed read '%s' results '%u'\n", fsPath.string().c_str(), nPakId);
							}
							else
							{
								g_nLoadedPakFileId.push_back(nPakId);
							}
						}
					}
				}
			}
		}
		catch (const std::exception& ex)
		{
			Warning(eDLL_T::RTECH, "Exception while parsing RPak load list: '%s'\n", ex.what());
			return;
		}
	}
}
