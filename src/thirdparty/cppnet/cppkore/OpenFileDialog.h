#pragma once

#include <cstdint>
#include "Control.h"
#include "ListBase.h"
#include "StringBase.h"
#include "DialogResult.h"

namespace Forms
{
	// Handles a dialog prompt that asks for a file or files to open
	class OpenFileDialog
	{
	public:
		static String ShowFolderDialog(const String& Title, const String& BasePath = "", Control* Owner = nullptr);

		// Opens the dialog and allows the user to select one file
		static String ShowFileDialog(const String& Title, const String& BasePath = "", const String& Filter = " |*.*", Control* Owner = nullptr);
		// Opens the dialog and allows the user to select multiple files
		static List<String> ShowMultiFileDialog(const String& Title, const String& BasePath = "", const String& Filter = " |*.*", Control* Owner = nullptr);
	};
}