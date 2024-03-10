#pragma once

#ifdef _DEBUG
#define LOG(x) std::cout << x << "\n";
#else
#define LOG(x)
#endif // _DEBUG

namespace Console
{
	void CreateConsole(const uint32_t maxConsoleLines);
	void ReleaseConsole();
}