#include "stdafx.h"
#include "Application.h"

// We're an application, so, we require GDI+, ComCtl32
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Comctl32.lib")

namespace Forms
{
	// We haven't initialized it yet...
	std::atomic<bool> Application::IsGdipInitialized = false;
	ULONG_PTR Application::GdipToken = NULL;

	void Application::Run(Form* MainWindow, bool DeleteWindow)
	{
		if (MainWindow == nullptr)
			return;

		// Initialize COM
		OleInitialize(NULL);

#if _DEBUG
		if (!IsGdipInitialized)
			printf("-- Warning: GDI+ has not been initialized before Application::Run() --\n");
#endif

		// Execute on the main loop
		Application::RunMainLoop(MainWindow);

		if (MainWindow && DeleteWindow)
			delete MainWindow;

		// Shutdown COM
		OleUninitialize();
	}

	void Application::RunDialog(Form* MainDialog)
	{
		// We must disable the owner dialog so that we can properly be the focus
		auto HwndOwner = GetWindowLongPtr(MainDialog->GetHandle(), GWLP_HWNDPARENT);
		if (HwndOwner != NULL)
		{
			if (IsWindowEnabled((HWND)HwndOwner))
				EnableWindow((HWND)HwndOwner, false);
		}

		RunMainLoop(MainDialog);

		// We must re-enable the owner window
		if (HwndOwner != NULL)
			EnableWindow((HWND)HwndOwner, true);
	}

	void Application::EnableVisualStyles()
	{
		SetProcessDPIAware();
		if (!IsGdipInitialized)
			InitializeGdip();
	}

	void Application::RunMainLoop(Form* MainWindow)
	{
		bool ContinueLoop = true;

		//
		// Initialize the main window before starting the message pump...
		//

		MainWindow->Show();

		//
		// Loop until any of the conditions are met...
		//

		MSG nMSG;
		while (ContinueLoop)
		{
			auto Peeked = PeekMessage(&nMSG, NULL, 0, 0, PM_NOREMOVE);

			if (Peeked)
			{
				if (!GetMessage(&nMSG, NULL, 0, 0))
					continue;

				TranslateMessage(&nMSG);
				DispatchMessage(&nMSG);

				if (MainWindow)
					ContinueLoop = !MainWindow->CheckCloseDialog(false);
			}
			else if (MainWindow == nullptr || MainWindow->GetState(ControlStates::StateDisposed))
			{
				break;
			}
			else if (!PeekMessage(&nMSG, NULL, 0, 0, PM_NOREMOVE))
			{
				WaitMessage();
			}
		}
	}

	void Application::InitializeGdip()
	{
		IsGdipInitialized = true;

		Gdiplus::GdiplusStartupInput gdipStartup;
		Gdiplus::GdiplusStartup(&GdipToken, &gdipStartup, NULL);
	}

	void Application::ShutdownGdip()
	{
		IsGdipInitialized = false;

		Gdiplus::GdiplusShutdown(GdipToken);
	}
}
