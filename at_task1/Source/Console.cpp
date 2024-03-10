#include "Pch.h"
#include "Console.h"

static bool consoleCreated{ false };

void Console::CreateConsole(const uint32_t maxConsoleLines)
{
	if (!consoleCreated)
	{
		CONSOLE_SCREEN_BUFFER_INFO console_info;
		FILE* fp;
		// allocate a console for this app
		AllocConsole();
		// set the screen buffer to be big enough to let us scroll text
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
		console_info.dwSize.Y = maxConsoleLines;
		SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), console_info.dwSize);
		// redirect unbuffered STDOUT to the console
		if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
			if (!(freopen_s(&fp, "CONOUT$", "w", stdout) != 0))
				setvbuf(stdout, NULL, _IONBF, 0);
		// redirect unbuffered STDIN to the console
		if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
			if (!(freopen_s(&fp, "CONIN$", "r", stdin) != 0))
				setvbuf(stdin, NULL, _IONBF, 0);
		// redirect unbuffered STDERR to the console
		if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
			if (!(freopen_s(&fp, "CONOUT$", "w", stderr) != 0))
				setvbuf(stderr, NULL, _IONBF, 0);
		// make C++ standard streams point to console as well
		std::ios::sync_with_stdio(true);
		// clear the error state for each of the C++ standard streams
		std::wcout.clear();
		std::cout.clear();
		std::wcerr.clear();
		std::cerr.clear();
		std::wcin.clear();
		std::cin.clear();

		consoleCreated = true;
	}
}

void Console::ReleaseConsole()
{
	if (consoleCreated)
	{
		FILE* fp;
		// Just to be safe, redirect standard IO to NULL before releasing.
		// Redirect STDIN to NULL
		if (!(freopen_s(&fp, "NUL:", "r", stdin) != 0))
			setvbuf(stdin, NULL, _IONBF, 0);
		// Redirect STDOUT to NULL
		if (!(freopen_s(&fp, "NUL:", "w", stdout) != 0))
			setvbuf(stdout, NULL, _IONBF, 0);
		// Redirect STDERR to NULL
		if (!(freopen_s(&fp, "NUL:", "w", stderr) != 0))
			setvbuf(stderr, NULL, _IONBF, 0);
		// Detach from console
		FreeConsole();
	}
}
