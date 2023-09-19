#include "core/stdafx.h"
#include "core/resource.h"
#include "windows/resource.h"

/*-----------------------------------------------------------------------------
 * _resource.cpp
 *-----------------------------------------------------------------------------*/

//#############################################################################
// 
//#############################################################################
MODULERESOURCE GetModuleResource(int iResource)
{
    static HGLOBAL rcData = NULL;
    HMODULE handle;

    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)"unnamed", &handle);

    HRSRC rc = FindResource(handle, MAKEINTRESOURCE(iResource), MAKEINTRESOURCE(PNG));
    if (!rc)
    {
        assert(rc == NULL);
        return MODULERESOURCE();
    }

    rcData = LoadResource(handle, rc);
    if (!rcData)
    {
        assert(rcData == NULL);
        return MODULERESOURCE();
    }
    return (MODULERESOURCE(LockResource(rcData), SizeofResource(handle, rc)));
}
///////////////////////////////////////////////////////////////////////////////