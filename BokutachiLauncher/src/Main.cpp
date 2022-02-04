#include <BokutachiLauncher/Injector.h>
#include <BokutachiLauncher/LaunchGame.h>
#include <BokutachiLauncher/GetAuth.h>
#include <iostream>
#include <fstream>

static void PrintError()
{
	std::cout << "Press \"Enter\" to continue..." << std::endl;
	std::cin.ignore();
}

int main()
{
	std::ofstream out("BokutachiHook.log");
	std::cout.rdbuf(out.rdbuf());


	if (GetAuth() != 0)
	{
		PrintError();
		return 1;
	}

	if (LaunchGame() != 0)
	{
		PrintError();
		return 1;
	}

	if (Injector() != 0)
	{
		PrintError();
		return 1;
	}

	return 0;
}