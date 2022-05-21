#include "stdafx.h"
#include "Icon.h"
#include "Path.h"
#include "Environment.h"
#include <shellapi.h>

namespace Drawing
{
	Icon::Icon()
		: _IconHandleSm(nullptr) ,_IconHandleLg(nullptr)
	{
	}

	Icon::Icon(HICON Icon)
		: _IconHandleSm(Icon), _IconHandleLg(Icon)
	{
	}

	Icon::~Icon()
	{
		if (_IconHandleSm != nullptr)
			DestroyIcon(_IconHandleSm);
		if (_IconHandleLg != nullptr)
			DestroyIcon(_IconHandleLg);

		_IconHandleLg = nullptr;
		_IconHandleLg = nullptr;
	}

	HICON Icon::LargeHandle()
	{
		return this->_IconHandleLg;
	}

	HICON Icon::SmallHandle()
	{
		return this->_IconHandleSm;
	}

	std::unique_ptr<Icon> Icon::FromFile(const string& File)
	{
		auto Result = std::make_unique<Icon>();

		Result->_IconHandleLg = (HICON)LoadImageA(NULL, (char*)File, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_LOADFROMFILE);
		Result->_IconHandleSm = (HICON)LoadImageA(NULL, (char*)File, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);

		return Result;
	}

	std::unique_ptr<Icon> Icon::FromResource(const int32_t ID)
	{
		auto Result = std::make_unique<Icon>();

		Result->_IconHandleLg = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(ID), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), NULL);
		Result->_IconHandleSm = (HICON)LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCEA(ID), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), NULL);

		return Result;
	}

	std::unique_ptr<Icon> Icon::ApplicationIcon()
	{
		auto Result = std::make_unique<Icon>();
		auto Exe = System::Environment::GetApplication();

		Result->_IconHandleLg = ExtractIconA(GetModuleHandle(NULL), (const char*)Exe, 0);
		Result->_IconHandleSm = Result->_IconHandleLg;

		return Result;
	}
}
