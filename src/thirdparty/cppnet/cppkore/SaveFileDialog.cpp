#include "stdafx.h"
#include "SaveFileDialog.h"
#include "Path.h"
#include <commdlg.h>

namespace Forms
{
	const String BuildSaveFileFilter(const String& Filter)
	{
		String InitialFilter = (String::IsNullOrWhiteSpace(Filter)) ? String(" |*.*") : Filter;
		auto Buffer = std::make_unique<int8_t[]>((size_t)InitialFilter.Length() + 2);	// Final filter has two null chars
		auto BufferMask = (char*)Buffer.get();

		std::memcpy(Buffer.get(), InitialFilter.ToCString(), InitialFilter.Length());

		for (uint32_t i = 0; i < InitialFilter.Length(); i++)
		{
			if (BufferMask[i] == '|')
				BufferMask[i] = (char)0;
		}

		BufferMask[InitialFilter.Length() + 1] = (char)0;

		return String((char*)Buffer.get(), (size_t)InitialFilter.Length() + 1);
	}

	String SaveFileDialog::ShowFileDialog(const String& Title, const String& BasePath, const String& Filter, Control* Owner)
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
			return String(Buffer);

		// We need nothing as a default because we don't get a return value
		return "";
	}
}
