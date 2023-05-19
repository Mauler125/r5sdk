//===========================================================================//
//
// Purpose: Model loading / unloading interface
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "engine/cmodel_bsp.h"
#include "engine/modelloader.h"
#include "datacache/mdlcache.h"

model_t* pErrorMDL = nullptr;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loader - 
//			*model - 
//-----------------------------------------------------------------------------
void CModelLoader::LoadModel(CModelLoader* loader, model_t* model)
{
	//if (!pErrorMDL)
	//{
	//	if (strcmp(model->szPathName, ERROR_MODEL) == 0)
	//	{
	//		pErrorMDL = model;
	//	}
	//}

	//string svExtension = model->szPathName;
	//size_t npos = svExtension.find(".");
	//if (npos != string::npos)
	//{
	//	svExtension = svExtension.substr(npos + 1);
	//}

	//if (strcmp(svExtension.c_str(), "rmdl") == 0 && strcmp(model->szPathName, ERROR_MODEL) != 0)
	//{
	//	studiohdr_t* pStudioHDR = g_MDLCache->FindMDL(g_MDLCache->m_pVTable, model->studio, 0);
	//	if (pStudioHDR == pErrorStudioHDR)
	//	{
	//		model = pErrorMDL;
	//	}
	//}
	return CModelLoader__LoadModel(loader, model);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *loader - 
//			*model - 
//-----------------------------------------------------------------------------
uint64_t CModelLoader::Map_LoadModelGuts(CModelLoader* loader, model_t* model)
{
	return CModelLoader__Map_LoadModelGuts(loader, model);
}

void CMapLoadHelper::Constructor(CMapLoadHelper* helper, int lumpToLoad)
{
	CMapLoadHelper__CMapLoadHelper(helper, lumpToLoad);
}

///////////////////////////////////////////////////////////////////////////////
void VModelLoader::Attach() const
{
	DetourAttach((LPVOID*)&CModelLoader__LoadModel, &CModelLoader::LoadModel);
	DetourAttach((LPVOID*)&CModelLoader__Map_LoadModelGuts, &CModelLoader::Map_LoadModelGuts);

	DetourAttach((LPVOID*)&CMapLoadHelper__CMapLoadHelper, &CMapLoadHelper::Constructor);
}

void VModelLoader::Detach() const
{
	DetourDetach((LPVOID*)&CModelLoader__LoadModel, &CModelLoader::LoadModel);
	DetourDetach((LPVOID*)&CModelLoader__Map_LoadModelGuts, &CModelLoader::Map_LoadModelGuts);

	DetourDetach((LPVOID*)&CMapLoadHelper__CMapLoadHelper, &CMapLoadHelper::Constructor);
}