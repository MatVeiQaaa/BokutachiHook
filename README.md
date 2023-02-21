# BokutachiHook
Hook for Lunatic Rave 2 to parse score data and send it to an HTTP server, made 
specifically for Bokutachi IR (https://bokutachi.xyz).

This project uses curl/curl and nlohmann/json as dependencies, they are required 
to compile it. As Lunatic Rave 2 is a 32-bit program, and resulting 
BokutachiHook.dll must be injected in its process, everything should be compiled 
targeting 32-bit platform.

CURL: https://github.com/curl/curl
nlohmann/json: https://github.com/nlohmann/json

For the project to work BokutachiAuth.json is required in the root folder, 
containing server URL and an API key - the file can be generated at 
https://bokutachi.xyz

Bokutachi project source: https://github.com/TNG-dev/Tachi

NOTE: As of v1.5.3 BokutachiLauncher allows the use of GAS (Gauge Auto Shift) optional feature. To enable it you have to put LR2GAS.dll in the Lunatic Rave 2 folder alongside with the launcher. You can get the LR2GAS.dll and project details here: https://github.com/aeventyr/LR2GAS_pub/releases/tag/v1

Also includes a patch for song preview memory leak, described at https://github.com/jbscj/lr2-memleak-fix