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
void MOD_PreloadPak()
{
	ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << g_pHostState->m_levelName << ".json";

	fs::path fsPath = std::filesystem::current_path() /= ostream.str();
	if (FileExists(fsPath.string().c_str()))
	{
		nlohmann::json jsIn;
		try
		{
			ifstream iPakLoadDefFile(fsPath.string().c_str(), std::ios::binary); // Load prerequisites file.

			jsIn = nlohmann::json::parse(iPakLoadDefFile);
			iPakLoadDefFile.close();

			if (!jsIn.is_null())
			{
				if (!jsIn["rpak"].is_null())
				{
					for (auto& it : jsIn["rpak"])
					{
						if (it.is_string())
						{
							string svToLoad = it.get<string>() + ".rpak";
							RPakHandle_t nPakId = g_pakLoadApi->AsyncLoad(svToLoad.c_str(), g_pMallocPool.GetPtr(), 4, 0);

							if (nPakId == -1)
								Error(eDLL_T::ENGINE, "%s: unable to load pak '%s' results '%d'\n", __FUNCTION__, svToLoad.c_str(), nPakId);
							else
								g_LoadedPakHandle.push_back(nPakId);
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
