#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <thread>

#include "BokutachiHook.hpp"
#include <LR2Mem/LR2Bindings.hpp>

static FILE* console_thread = nullptr;

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
#ifdef _DEBUG
        AllocConsole();
        freopen_s(&console_thread, "CONOUT$", "w", stdout);

        std::cout << "Debug Console Attached" << std::endl;

        while (!IsDebuggerPresent())
            Sleep(100);
#endif
        LR2::Init();
        std::thread(BokutachiHook::Init).detach();

        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved != nullptr) {
            break;
        }
        BokutachiHook::Deinit();
#ifdef _DEBUG
        fclose(console_thread);
        FreeConsole();
#endif

        break;
    }
    return TRUE;
}
