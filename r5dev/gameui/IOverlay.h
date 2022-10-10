#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "public/isurfacesystem.h"
#include "thirdparty/imgui/include/imgui_logger.h"
#include "thirdparty/imgui/include/imgui_utility.h"

class COverlay : public ISurface
{
public:
    ///////////////////////////////////////////////////////////////////////////
    COverlay(void);
    virtual ~COverlay(void);

    virtual bool Init(void);
    virtual void Think(void);

    virtual void RunFrame(void);
    virtual void RunTask(void);

    virtual void DrawSurface(void);
    virtual void DrawHint(void);

private:

    ///////////////////////////////////////////////////////////////////////////
    virtual void SetStyleVar(void);

private:
    ///////////////////////////////////////////////////////////////////////////
    float                          m_flFadeAlpha      = 0.f;

    bool                           m_bInitialized     = false;
    bool                           m_bReclaimFocus    = false;

    ID3D11ShaderResourceView* m_idR5RIcon = nullptr;
    MODULERESOURCE                 m_rR5RIconBlob;

    ImGuiStyle_t                   m_Style = ImGuiStyle_t::NONE;
    
public:
    bool             m_bActivate = false;
    bool             m_bHintShown = false;
    bool             m_bConsole = false;
    bool             m_bServerList = false;
    bool             m_bHosting = false;
    bool             m_bPlayerList = false;
    bool             m_bSettings = false;
};

///////////////////////////////////////////////////////////////////////////////
extern COverlay* g_pOverlay;
#endif // !DEDICATED
