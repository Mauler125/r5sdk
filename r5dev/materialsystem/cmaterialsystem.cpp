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
	fs::path fsPath = fs::current_path() /= ostream.str();

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

//---------------------------------------------------------------------------------
// Purpose: draw frame
//---------------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint_8t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12)
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, int64_t a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, int a11, __m128* a12, int a13, int64_t a14)
#endif
{
	// This only happens when the BSP is in a horrible condition (bad depth buffer draw calls!)
	// but allows you to load BSP's with virtually all missing shaders/materials and models 
	// being replaced with 'material_for_aspect/error.rpak' and 'mdl/error.rmdl'.
	if (!s_pRenderContext.GetValue<void*>())
		return nullptr;
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	return v_DispatchDrawCall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	return v_DispatchDrawCall(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void CMaterialSystem_Attach()
{
	DetourAttach((LPVOID*)&StreamDB_Init, &HStreamDB_Init);
	DetourAttach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
}

void CMaterialSystem_Detach()
{
	DetourDetach((LPVOID*)&StreamDB_Init, &HStreamDB_Init);
	DetourDetach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
}