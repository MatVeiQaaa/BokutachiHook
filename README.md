# BokutachiHook

Hook for Lunatic Rave 2 to parse score data and send it to an HTTP server, made 
specifically for [Bokutachi IR](https://boku.tachi.ac/).

For the project to work BokutachiAuth.json is required in the root folder, 
containing server URL and an API key - the file can be generated at 
https://boku.tachi.ac/client-file-flow/CXLR2Hook

The .dll file must be injected into the game process in any convenient way. It is recommended to use
https://github.com/SayakaIsBaka/lr2_chainload for automatic injection on game launch.
Add a new line 'BokutachiHook.dll' to 'chainload.txt' for it to be injected.

BokutachiHook optionally integrates with [LR2HackBox](https://github.com/MatVeiQaaa/LR2HackBox) and [LR2GAS](https://github.com/aeventyr/LR2GAS_pub).
Once their DLLs are injected, they are used to enhance the IR data.

## Building

This project uses vcpkg, so make sure to have it available in your terminal session,
and run 'vcpkg intergate install' to be able to build it.

Tachi project source: https://github.com/zkrising/Tachi