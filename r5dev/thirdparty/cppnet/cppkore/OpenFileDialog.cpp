#include "stdafx.h"
#include "OpenFileDialog.h"
#include "Path.h"
#include <commdlg.h>
#include <ShlObj.h>

namespace Forms
{
	const String BuildOpenFileFilter(const String& Filter)
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

	String OpenFileDialog::ShowFolderDialog(const String& Title, const String& BasePath, Control* Owner)
	{
		HWND OwnerHandle = (Owner != nullptr) ? Owner->GetHandle() : NULL;

		char path[MAX_PATH];
		BROWSEINFOA bi;

		bi.hwndOwner = OwnerHandle;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = (LPSTR)path;
		bi.lpszTitle = Title;
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		bi.lpfn = NULL;
		bi.lParam = 0;
		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl != NULL && SHGetPathFromIDListA(pidl, path))
			return String(path);

		// We need nothing as a default because we don't get a return value
		return "";
	}

	String OpenFileDialog::ShowFileDialog(const String& Title, const String& BasePath, const String& Filter, Control* Owner)
	{
		HWND OwnerHandle = (Owner != nullptr) ? Owner->GetHandle() : NULL;
		char Buffer[MAX_PATH]{};

		OPENFILENAMEA oFileDialog{};
		oFileDialog.lStructSize = sizeof(OPENFILENAMEA);

		auto FilterBuffer = BuildOpenFileFilter(Filter);

		oFileDialog.hwndOwner = OwnerHandle;
		oFileDialog.lpstrFilter = (LPSTR)FilterBuffer.ToCString();
		oFileDialog.lpstrInitialDir = (LPCSTR)BasePath.ToCString();
		oFileDialog.lpstrFile = (LPSTR)Buffer;
		oFileDialog.nMaxFile = MAX_PATH;
		oFileDialog.lpstrTitle = Title.ToCString();
		oFileDialog.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;

		// Open the dialog with the config and then return result
		if (GetOpenFileNameA(&oFileDialog))
			return String(Buffer);

		// We need nothing as a default because we don't get a return value
		return "";
	}

	List<String> OpenFileDialog::ShowMultiFileDialog(const String& Title, const String& BasePath, const String& Filter, Control* Owner)
	{
		HWND OwnerHandle = (Owner != nullptr) ? Owner->GetHandle() : NULL;
		char Buffer[0x2000]{};

		static bool bFileLessPath = false;

		OPENFILENAMEA oFileDialog{};
		oFileDialog.lStructSize = sizeof(OPENFILENAMEA);

		auto FilterBuffer = BuildOpenFileFilter(Filter);

		oFileDialog.hwndOwner = OwnerHandle;
		oFileDialog.lpstrFilter = (LPSTR)FilterBuffer.ToCString();
		oFileDialog.lpstrInitialDir = (LPCSTR)BasePath.ToCString();
		oFileDialog.lpstrFile = (LPSTR)Buffer;
		oFileDialog.nMaxFile = 0x2000;
		oFileDialog.lpstrTitle = Title.ToCString();
		oFileDialog.Flags = OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_EXPLORER;

		List<String> ResultList;

		// Open the dialog and parse each result
		if (GetOpenFileNameA(&oFileDialog))
		{
			const char* Path = oFileDialog.lpstrFile;

			auto BasePath = String(Path);

			while (*Path)
			{
				if (!bFileLessPath)
				{
					Path += ((size_t)BasePath.Length() + 1);
					bFileLessPath = true;
				}

				auto FileName = String(Path);
				ResultList.EmplaceBack(IO::Path::Combine(BasePath, FileName));
				Path += ((size_t)FileName.Length() + 1);
			}
		}

		bFileLessPath = false;
		return ResultList; // We need an empty list as default because we didn't get a return value
	}
}
