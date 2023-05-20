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
#ifndef DEDICATED
#include <vgui/vgui_baseui_interface.h>
#endif // !DEDICATED
#include <filesystem/filesystem.h>

model_t* pErrorMDL = nullptr;

//-----------------------------------------------------------------------------
// Purpose: checks if the lump type is valid and used
// Input  : lumpType - 
//-----------------------------------------------------------------------------
bool IsLumpIdxValid(int lumpType)
{
	switch (lumpType)
	{
	case LUMP_PLANES:
	case LUMP_VERTICES:
	case LUMP_SHADOW_ENVIRONMENTS:
	case LUMP_SURFACE_NAMES:
	case LUMP_CONTENTS_MASKS:
	case LUMP_SURFACE_PROPERTIES:
	case LUMP_BVH_NODES:
	case LUMP_BVH_LEAF_DATA:
	case LUMP_PACKED_VERTICES:
	case LUMP_VERTEX_NORMALS:
	case LUMP_UNKNOWN_37:
	case LUMP_UNKNOWN_38:
	case LUMP_UNKNOWN_39:
	case LUMP_VERTEX_UNLIT:
	case LUMP_VERTEX_LIT_FLAT:
	case LUMP_VERTEX_LIT_BUMP:
	case LUMP_VERTEX_UNLIT_TS:
	case LUMP_MESH_INDICES:
	case LUMP_LIGHTMAP_DATA_SKY:
	case LUMP_CSM_AABB_NODES:
	case LUMP_CSM_OBJ_REFERENCES:
	case LUMP_LIGHTPROBES:
	case LUMP_LIGHTPROBE_TREE:
	case LUMP_LIGHTPROBE_REFERENCES:
	case LUMP_LIGHTMAP_DATA_REAL_TIME_LIGHTS:
	case LUMP_CELL_BSP_NODES:
	case LUMP_CELLS:
	case LUMP_PORTALS:
	case LUMP_PORTAL_VERTICES:
	case LUMP_PORTAL_EDGES:
	case LUMP_PORTAL_VERTEX_EDGES:
	case LUMP_PORTAL_VERTEX_REFERENCES:
	case LUMP_PORTAL_EDGE_REFERENCES:
	case LUMP_PORTAL_EDGE_INTERSECT_AT_EDGE:
	case LUMP_PORTAL_EDGE_INTERSECT_AT_VERTEX:
	case LUMP_PORTAL_EDGE_INTERSECT_HEADER:
	case LUMP_OCCLUSION_MESH_VERTICES:
	case LUMP_OCCLUSION_MESH_INDICES:
	case LUMP_CELL_AABB_NODES:
	case LUMP_OBJ_REFERENCES:
	case LUMP_OBJ_REFERENCE_BOUNDS:
	case LUMP_SHADOW_MESH_OPAQUE_VERTICES:
	case LUMP_SHADOW_MESH_INDICES:
	case LUMP_SHADOW_MESHES:
		return true;
	default:
		return false;
	}
}

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

void CMapLoadHelper::Constructor(CMapLoadHelper* loader, int lumpToLoad)
{
#ifndef DEDICATED
	g_pEngineVGui->UpdateProgressBar(PROGRESS_DEFAULT);
#endif // !DEDICATED

	if (lumpToLoad > HEADER_LUMPS-1)
		Error(eDLL_T::ENGINE, EXIT_FAILURE, "Can't load lump %i, range is 0 to %i!!!", lumpToLoad, HEADER_LUMPS-1);

	loader->m_nLumpID = lumpToLoad;
	loader->m_nLumpSize = 0;
	loader->m_pData = nullptr;
	loader->m_pRawData = nullptr;
	loader->m_pUncompressedData = nullptr;
	loader->m_nUncompressedLumpSize = 0;
	loader->m_bUncompressedDataExternal = 0;
	loader->m_bExternal = false;
	loader->m_bUnk = false;
	loader->m_nLumpOffset = -1;

	if (lumpToLoad <= s_MapHeader->lastLump)
	{
		lump_t* lump = &s_MapHeader->lumps[lumpToLoad];

		int lumpOffset = lump->fileofs;
		if (!lumpOffset)
			return; // Idk if this is correct.

		int lumpSize = lump->filelen;
		if (lumpSize)
		{
			loader->m_nLumpSize = lumpSize;
			loader->m_nLumpOffset = lump->fileofs;
			loader->m_nLumpVersion = lump->version;

			FileHandle_t mapFileHandle = *s_MapFileHandle;

			if (mapFileHandle == FILESYSTEM_INVALID_HANDLE)
			{
				Error(eDLL_T::ENGINE, EXIT_FAILURE, "Can't load map from invalid handle!!!");
				lumpSize = loader->m_nLumpSize;
			}

			loader->m_nUncompressedLumpSize = lumpSize;

			char pathBuf[MAX_PATH];
			FileSystemCache fileCache;

			fileCache.pBuffer = nullptr;

			if (IsLumpIdxValid(lumpToLoad)
				&& (V_snprintf(pathBuf, sizeof(pathBuf), "%s.%.4X.bsp_lump", s_szMapPathName, lumpToLoad), FileSystem()->ReadFromCache(pathBuf, &fileCache)))
			{
				loader->m_pRawData = nullptr;
				loader->m_pData = fileCache.pBuffer->pData;
				loader->m_bExternal = true;
				loader->m_bUnk = fileCache.pBuffer->nUnk0 == 0;
			}
			else
			{
				int bytesToRead = lumpSize ? lumpSize : 1;
				loader->m_pRawData = MemAllocSingleton()->Alloc<byte>(bytesToRead);

				if (loader->m_nLumpSize)
				{
					FileHandle_t hLumpFile = FileSystem()->Open(pathBuf, "rb");

					loader->m_pData = loader->m_pRawData;

					if (hLumpFile != FILESYSTEM_INVALID_HANDLE)
					{
						FileSystem()->ReadEx(loader->m_pRawData, bytesToRead, bytesToRead, hLumpFile);
						FileSystem()->Close(hLumpFile);

						loader->m_pRawData = nullptr;
						loader->m_bExternal = true;
					}
					else // Seek to offset in packed BSP file to load the lump.
					{
						FileSystem()->Seek(mapFileHandle, loader->m_nLumpOffset, FILESYSTEM_SEEK_HEAD);
						FileSystem()->ReadEx(loader->m_pRawData, bytesToRead, bytesToRead, mapFileHandle);
					}
				}
			}
		}
	}
	else
	{
		loader->m_nLumpSize = 0;
		loader->m_nLumpOffset = 0;
		loader->m_nLumpVersion = 0;
	}
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