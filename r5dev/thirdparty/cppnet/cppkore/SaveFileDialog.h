#pragma once

#include <cstdint>
#include "Control.h"
#include "ListBase.h"
#include "StringBase.h"
#include "DialogResult.h"

namespace Forms
{
	// Handles a dialog prompt that asks for a file to save
	class SaveFileDialog
	{
	public:
		// Opens the dialog and allows the user to save one file
		static String ShowFileDialog(const String& Title, const String& BasePath = "", const String& Filter = " |*.*", Control* Owner = nullptr);
	};
}
