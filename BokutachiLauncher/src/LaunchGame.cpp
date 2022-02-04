#include <BokutachiLauncher/LaunchGame.h>
#include <Windows.h>
#include <fstream>
#include <iostream>

int LaunchGame()
{
	std::ofstream out("BokutachiHook.log");
	std::cout.rdbuf(out.rdbuf());

	STARTUPINFO si{};
	PROCESS_INFORMATION pi{};
	
	if (CreateProcess("LR2body.exe", nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != 1)
	{
		if (CreateProcess("LRHbody.exe", nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != 1)
		{
			std::cout << "Couldn't launch LR2body.exe or LRHbody.exe\n";
			return 1;
		}
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}