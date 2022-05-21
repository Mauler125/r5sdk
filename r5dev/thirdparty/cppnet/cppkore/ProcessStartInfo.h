#pragma once

#include "StringBase.h"

namespace Diagnostics
{
	enum class ProcessWindowStyle
	{
		Normal,
		Hidden,
		Minimized,
		Maximized
	};

	// Information used when starting a new process
	struct ProcessStartInfo
	{
		// Sets the application, document, or URL that is to be launched.
		string FileName;
		// Specifies the set of command line arguments to use when starting the application.
		string Arguments;
		// Sets the initial directory for the process that is started.
		string WorkingDirectory;
		// Specifies the verb to use when opening the filename.
		string Verb;

		// Whether or not to allow window creation.
		bool CreateNoWindow;
		// Whether to use ShellExecute vs CreateProcess.
		bool UseShellExecute;

		// Sets the style of window that should be used for the newly created process.
		ProcessWindowStyle WindowStyle;

		ProcessStartInfo(const string& FileName)
			: FileName(FileName), UseShellExecute(true), CreateNoWindow(false), WindowStyle(ProcessWindowStyle::Normal)
		{
		}

		ProcessStartInfo(const string& FileName, const string& Arguments)
			: FileName(FileName), Arguments(Arguments), UseShellExecute(true), CreateNoWindow(false), WindowStyle(ProcessWindowStyle::Normal)
		{
		}
	};
}