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
		static string ShowFileDialog(const string& Title, const string& BasePath = "", const string& Filter = " |*.*", Control* Owner = nullptr);
	};
}
