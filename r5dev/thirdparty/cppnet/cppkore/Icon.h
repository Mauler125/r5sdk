#pragma once

#include <cstdint>
#include <memory>
#include "StringBase.h"
#include "DrawingBase.h"

namespace Drawing
{
	// Represents a drawable windows icon.
	class Icon
	{
	public:
		Icon();
		Icon(HICON Icon);
		~Icon();

		// Gets a handle to the large rendition of this icon.
		HICON LargeHandle();
		// Gets a handle to the small rendition of this icon.
		HICON SmallHandle();

		// Loads an icon resouce from a file.
		static std::unique_ptr<Icon> FromFile(const string& File);
		// Loads an icon resource from a resource.
		static std::unique_ptr<Icon> FromResource(const int32_t ID);
		// Loads the application defined icon.
		static std::unique_ptr<Icon> ApplicationIcon();

	private:
		// The internal icon handles
		HICON _IconHandleSm;
		HICON _IconHandleLg;
	};
}