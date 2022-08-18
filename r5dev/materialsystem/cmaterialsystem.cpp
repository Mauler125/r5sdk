//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "rtech/rtech_utils.h"
#include "materialsystem/cmaterialsystem.h"

//---------------------------------------------------------------------------------
// Purpose: loads and processes STBSP files
// (overrides level name if stbsp field has value in prerequisites file)
// Input  : *pszLevelName - 
//---------------------------------------------------------------------------------
void StreamDB_Init(const char* pszLevelName)
{
	ostringstream ostream;
	ostream << "platform\\scripts\\levels\\settings\\" << pszLevelName << ".json";
	fs::path fsPath = fs::current_path() /= ostream.str();

	if (FileExists(fsPath))
	{
		nlohmann::json jsIn;
		try
		{
			ifstream iPakLoadDefFile(fsPath, std::ios::binary); // Parse prerequisites file.
			iPakLoadDefFile >> jsIn;
			iPakLoadDefFile.close();

			if (!jsIn.is_null())
			{
				if (!jsIn[STREAM_DB_EXT].is_null())
				{
					string svStreamDBFile = jsIn[STREAM_DB_EXT].get<string>();
					DevMsg(eDLL_T::MS, "%s: Loading override STBSP file '%s.%s'\n", __FUNCTION__, svStreamDBFile.c_str(), STREAM_DB_EXT);
					v_StreamDB_Init(svStreamDBFile.c_str());
					return;
				}
			}
		}
		catch (const std::exception& ex)
		{
			Warning(eDLL_T::MS, "%s: Exception while parsing STBSP override: '%s'\n", __FUNCTION__, ex.what());
		}
	}
	DevMsg(eDLL_T::MS, "%s: Loading STBSP file '%s.%s'\n", __FUNCTION__, pszLevelName, STREAM_DB_EXT);
	v_StreamDB_Init(pszLevelName);
}

//---------------------------------------------------------------------------------
// Purpose: draw frame
//---------------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
void* __fastcall DispatchDrawCall(int64_t a1, uint64_t a2, int a3, int a4, char a5, int a6, uint8_t a7, int64_t a8, uint32_t a9, uint32_t a10, __m128* a11, int a12)
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
	DetourAttach((LPVOID*)&v_StreamDB_Init, &StreamDB_Init);
	DetourAttach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
}

void CMaterialSystem_Detach()
{
	DetourDetach((LPVOID*)&v_StreamDB_Init, &StreamDB_Init);
	DetourDetach((LPVOID*)&v_DispatchDrawCall, &DispatchDrawCall);
}