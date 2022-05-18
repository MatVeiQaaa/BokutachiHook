#include <BokutachiLauncher/Injector.h>

#include <iostream>
#include <fstream>

#include <windows.h>

#include <tlhelp32.h>

// Iterate over the list of process names until procName is found.
static DWORD GetProcId(const char* procName)
{
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry) != 0)
		{
			do
			{
				if (strcmp(procEntry.szExeFile, procName) == 0)
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));
		}
	}
	CloseHandle(hSnap);
	return procId;
}

int Injector()
{
	std::ofstream out("BokutachiHook.log");
	std::cout.rdbuf(out.rdbuf());

	constexpr const char* dllPath = "BokutachiHook.dll";
	constexpr const char* procName = "LR2body.exe";
	DWORD procId = 0;

	procId = GetProcId(procName);
	if (procId == 0)
	{
		procId = GetProcId("LRHbody.exe");
		if (procId == 0)
		{
			std::cout << "Couldn't find LR2body.exe process\n";
			return 1;
		}
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

	if (hProc == nullptr || hProc == INVALID_HANDLE_VALUE)
	{
		std::cout << "LR2body.exe is found, but couldn't get a handle to it\n";
		return 1;
	}

	void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (loc == nullptr)
	{
		std::cout << "Couldn't allocate memory in the remote process\n";
		return 1;
	}

	if (WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0) == 0)
	{
		std::cout << "Couldn't write .dll to LR2 memory\n";
		return 1;
	}


	HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);

	if (hThread != nullptr)
	{
		CloseHandle(hThread);
	}

	else
	{
		std::cout << "Couldn't start remote thread of the .dll\n";
		return 1;
	}

	CloseHandle(hProc);

	return 0;
}