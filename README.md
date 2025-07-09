# BokutachiHook
Hook for Lunatic Rave 2 to parse score data and send it to an HTTP server, made 
specifically for Bokutachi IR (https://boku.tachi.ac/).

For the project to work BokutachiAuth.json is required in the root folder, 
containing server URL and an API key - the file can be generated at 
https://boku.tachi.ac/client-file-flow/CXLR2Hook

The .dll file must be injected into the game process in any convenient way. I recommend using
https://github.com/SayakaIsBaka/lr2_chainload for automatic injection on game launch.
You would need to add 'BokutachiHook.dll' to 'chainload.txt' for it to be injected.

This project uses vcpkg, make sure to have it globally available in your system,
and run 'vcpkg intergate install' in your terminal session to be able to build it.

Tachi project source: https://github.com/zkrising/Tachi