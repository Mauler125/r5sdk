#pragma once
#include <engine/server/sv_main.h>
#include <vguimatsurface/MatSystemSurface.h>

#define VGUI_RICHTEXT_MAX_LEN 8192

namespace vgui
{
	class RichText;

	struct vgui_RichText_vtbl
	{
		void* funcs_0[239];
		__int64(__fastcall* SetText)(vgui::RichText*, WCHAR*);
		void* funcs_780[43];
		__int64(__fastcall* ResolveLocalizedTextAndVariables)(vgui::RichText*, const CHAR*, WCHAR*, __int64);
	};

	class RichText
	{
	public:
		vgui_RichText_vtbl* __vftable;

	public:
		void SetText(const char* text);
	};
};

/* ==== RICHTEXT ===================================================================================================================================================== */
inline CMemory p_vgui_RichText_SetText;
inline void(*vgui_RichText_SetText)(vgui::RichText* thisptr, const char* text);

///////////////////////////////////////////////////////////////////////////////
class VVGUIRichText : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("vgui::RichText::SetText", p_vgui_RichText_SetText.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_vgui_RichText_SetText = g_GameDll.FindPatternSIMD("40 53 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9");
		vgui_RichText_SetText = p_vgui_RichText_SetText.RCast<void(*)(vgui::RichText*, const char*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
