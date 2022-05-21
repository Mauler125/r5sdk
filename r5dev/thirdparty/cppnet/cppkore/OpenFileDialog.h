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
		static string ShowFolderDialog(const string& Title, const string& BasePath = "", Control* Owner = nullptr);

		// Opens the dialog and allows the user to select one file
		static string ShowFileDialog(const string& Title, const string& BasePath = "", const string& Filter = " |*.*", Control* Owner = nullptr);
		// Opens the dialog and allows the user to select multiple files
		static List<string> ShowMultiFileDialog(const string& Title, const string& BasePath = "", const string& Filter = " |*.*", Control* Owner = nullptr);
	};
}