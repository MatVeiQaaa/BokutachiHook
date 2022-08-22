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

Bokutachi project source: https://github.com/TNG-dev/Tachu
