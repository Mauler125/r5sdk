//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/sys_utils.h"
#include "materialsystem/cmaterialsystem.h"

//---------------------------------------------------------------------------------
// Purpose: loads and processes STBSP files
// (overrides level name if stbsp field has value in prerequisites file)
// Input  : *pszStreamDBFile - 
//---------------------------------------------------------------------------------
void HStreamDB_Init(const char* pszStreamDBFile)
{
	std::ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << pszStreamDBFile << ".json";
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
				if (!jsIn["stbsp"].is_null())
				{
					std::string svStreamDBFile = jsIn["stbsp"].get<std::string>();
					DevMsg(eDLL_T::MS, "StreamDB_Init: Loading override STBSP file '%s.stbsp'\n", svStreamDBFile.c_str(), pszStreamDBFile);
					StreamDB_Init(svStreamDBFile.c_str());
					return;
				}
			}
		}
		catch (const std::exception& ex)
		{
			DevMsg(eDLL_T::MS, "StreamDB_Init: Exception while parsing STBSP override: '%s'\n", ex.what());
		}
	}
	StreamDB_Init(pszStreamDBFile);
}

///////////////////////////////////////////////////////////////////////////////
void CMaterialSystem_Attach()
{
	DetourAttach((LPVOID*)&StreamDB_Init, &HStreamDB_Init);
}

void CMaterialSystem_Detach()
{
	DetourDetach((LPVOID*)&StreamDB_Init, &HStreamDB_Init);
}