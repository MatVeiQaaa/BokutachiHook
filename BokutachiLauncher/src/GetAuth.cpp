#include <BokutachiLauncher/GetAuth.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

int GetAuth()
{
	std::ofstream out("BokutachiHook.log");
	std::cout.rdbuf(out.rdbuf());

	std::ifstream conf("BokutachiAuth.json");
	if (!conf.good())
	{
		std::cout << "Config file is absent\n";
		return 1;
	}

	json config = json::parse(conf);
	std::string api = config.at("apiKey");
	if (api.length() != 40 || config.at("url").empty())
	{
		std::cout << "Config file is wrong\n";
		return 1;
	}

	return 0;
}