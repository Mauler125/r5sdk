//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "materialsystem/cshaderglue.h"

int CShaderGlue::SetupShader(uint64_t nCount, uint64_t a3, void* pRawMaterialGlueWithoutVTable)
{
	return CShaderGlue_SetupShader(this, nCount, a3, pRawMaterialGlueWithoutVTable);
}
