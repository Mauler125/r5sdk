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

int g_nLoadedPakFileId[256]{};

//-----------------------------------------------------------------------------
// Purpose: loads required assets for specified level
// Todo   : STBSP and VPK loading
// Todo   : Unload all assets loaded by this functions using 'nPakId'
//-----------------------------------------------------------------------------
void MOD_LoadDependencies(eBspRes_t resourceType)
{
	std::ostringstream ostream;
	ostream << "platform\\scripts\\levels\\" << g_pHostState->m_levelName << ".json";

	std::filesystem::path fsPath = std::filesystem::current_path() /= ostream.str();
	DevMsg(eDLL_T::ENGINE, "Loading level prerequisites file: '%s'\n", fsPath.string().c_str());

	if (FileExists(fsPath.string().c_str()))
	{
		nlohmann::json jsIn;
		try
		{
			std::ifstream iPakLoadDefFile(fsPath, std::ios::binary); // Parse prerequisites file.
			iPakLoadDefFile >> jsIn;
			iPakLoadDefFile.close();

			switch (resourceType)
			{
			case eBspRes_t::RES_RPAK:
			{
				if (!jsIn.is_null())
				{
					if (!jsIn["rpak"].is_null())
					{
						int iPakIdx{};
						for (auto it = jsIn["rpak"].begin(); it != jsIn["rpak"].end(); ++it)
						{
							if (it.value().is_string())
							{
								std::string svToLoad = it.value().get<std::string>() + ".rpak";
								unsigned int nPakId = 0;
								nPakId = RTech_AsyncLoad((void*)svToLoad.c_str(), g_pMallocPool.GetPtr(), 4, 0);

								if (nPakId == -1)
								{
									DevMsg(eDLL_T::RTECH, "RTech AsyncLoad failed read '%s' results '%u'\n", fsPath.string().c_str(), nPakId);
								}
								else
								{
									g_nLoadedPakFileId[iPakIdx] = nPakId;
									iPakIdx++;
								}
							}
						}
					}
				}
				break;
			}
			default:
			{
				break;
			}
			}
		}
		catch (const std::exception& ex)
		{
			DevMsg(eDLL_T::ENGINE, "Error parsing level prerequisites file: '%s'\n", fsPath.string().c_str(), ex.what());
			return;
		}
	}
}
