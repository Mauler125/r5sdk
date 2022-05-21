#include "stdafx.h"
#include "SaveFileDialog.h"
#include "Path.h"
#include <commdlg.h>

namespace Forms
{
	const string BuildSaveFileFilter(const string& Filter)
	{
		string InitialFilter = (string::IsNullOrWhiteSpace(Filter)) ? string(" |*.*") : Filter;
		auto Buffer = std::make_unique<int8_t[]>((size_t)InitialFilter.Length() + 2);	// Final filter has two null chars
		auto BufferMask = (char*)Buffer.get();

		std::memcpy(Buffer.get(), InitialFilter.ToCString(), InitialFilter.Length());

		for (uint32_t i = 0; i < InitialFilter.Length(); i++)
		{
			if (BufferMask[i] == '|')
				BufferMask[i] = (char)0;
		}

		BufferMask[InitialFilter.Length() + 1] = (char)0;

		return string((char*)Buffer.get(), (size_t)InitialFilter.Length() + 1);
	}

	string SaveFileDialog::ShowFileDialog(const string& Title, const string& BasePath, const string& Filter, Control* Owner)
	{
		HWND OwnerHandle = (Owner != nullptr) ? Owner->GetHandle() : NULL;
		char Buffer[MAX_PATH]{};

		OPENFILENAMEA oFileDialog{};
		oFileDialog.lStructSize = sizeof(OPENFILENAMEA);

		auto FilterBuffer = BuildSaveFileFilter(Filter);

		oFileDialog.hwndOwner = OwnerHandle;
		oFileDialog.lpstrFilter = (LPSTR)FilterBuffer.ToCString();
		oFileDialog.lpstrInitialDir = (LPCSTR)BasePath.ToCString();
		oFileDialog.lpstrFile = (LPSTR)Buffer;
		oFileDialog.nMaxFile = MAX_PATH;
		oFileDialog.lpstrTitle = Title.ToCString();

		// Open the dialog with the config and then return result
		if (GetSaveFileNameA(&oFileDialog))
			return string(Buffer);

		// We need nothing as a default because we don't get a return value
		return "";
	}
}
