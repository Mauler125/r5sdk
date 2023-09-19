#include "stdafx.h"
#include "UIXTheme.h"
#include "MessageBox.h"

#include "CheckBoxImage.h"

namespace UIX
{
	//
	// The theme renderer instance
	//

	static UIXRenderer* _RendererInstance;

	// Whether or not we have a theme setup
	static bool _ThemeInitialized = false;

	void UIXTheme::InitializeRenderer(UIXRenderer* Renderer)
	{
		if (_ThemeInitialized)
			ShutdownRenderer();

		_RendererInstance = Renderer;
		_ThemeInitialized = true;
	}

	void UIXTheme::ShutdownRenderer()
	{
		if (_ThemeInitialized)
		{
			delete _RendererInstance;

			_ThemeInitialized = false;
		}
	}

	UIXRenderer const* UIXTheme::GetRenderer()
	{
		return _RendererInstance;
	}
}