#include "BokutachiHook.hpp"

#include <print>
#include <format>
#include <fstream>
#include <thread>
#include <vector>

#include <LR2Mem/LR2Bindings.hpp>
#include <safetyhook.hpp>
#include <cpr/cpr.h>
#include <json/single_include/nlohmann/json.hpp>

#pragma comment(lib,"LR2Mem.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

using json = nlohmann::ordered_json;

static uintptr_t moduleBase = 0;

static std::string url;
static std::string urlDan;
static std::string apiKey;

constexpr const char* lamps[6] = { "NO PLAY", "FAIL", "EASY", "NORMAL", "HARD", "FULL COMBO" };
constexpr const char* gauges[6] = { "GROOVE", "HARD", "HAZARD", "EASY", "P-ATTACK", "G-ATTACK" };
constexpr const char* gameModes[8] = { "ALL", "SINGLE", "7K", "5K", "DOUBLE", "14K", "10K", "9K" };
constexpr const char* randomModes[6] = { "NORAN", "MIRROR", "RAN", "S-RAN", "H-RAN", "ALLSCR" };

static std::mutex notificationsMutex;
static std::vector<std::pair<std::string, std::chrono::time_point<std::chrono::system_clock>>> notifications;

static void AddNotification(std::string message) {
	const std::lock_guard lock(notificationsMutex);
	notifications.push_back({ message, std::chrono::system_clock::now() });
}

static void UpdateNotifications() {
	std::vector<decltype(notifications.begin())> toDelete;
	const std::lock_guard lock(notificationsMutex);
	for (auto it = notifications.begin(); it != notifications.end(); it++) {
		auto& [message, time] = *it;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time).count() > 3000) {
			toDelete.push_back(it);
		}
		else {
			typedef int(__cdecl* tPrintfDx)(const char* fmt, ...);
			tPrintfDx PrintfDx = (tPrintfDx)0x4C94B0;
			PrintfDx("%s\n", message.c_str());
		}
	}
	for (auto& it : toDelete) {
		notifications.erase(it);
	}
}

static safetyhook::MidHook OnBeforeScreenFlipHk;
static void OnBeforeScreenFlip(SafetyHookContext& regs) {
	UpdateNotifications();
}

void Logger(std::string message)
{
	std::ofstream logFile;
	logFile.open("Bokutachi.log", std::ios_base::app);

	if (static_cast<void*>(GetProcAddress(GetModuleHandle("ntdll"), "wine_get_version")) != nullptr) {
		logFile << std::format("[{:%d-%m-%Y %X}] {}\n", std::chrono::system_clock::now(), message);
	}
	else {
		auto const time = std::chrono::current_zone()
			->to_local(std::chrono::system_clock::now());
			logFile << std::format("[{:%d-%m-%Y %X}] {}\n", time, message);
	}
	logFile.close();
}

void CheckTachiApi() {
	std::string baseUrl = url.substr(0, url.find_first_of("/", 8));
	cpr::Response r = cpr::Get(cpr::Url{ baseUrl + "/api/v1/status" },
							   cpr::Bearer{ apiKey });

	if (r.error.code != cpr::ErrorCode::OK) {
		std::println("[BokutachiHook] Coulnd't GET: {}", r.error.message);
		AddNotification("Couldn't Connect to BokutachiIR!");
		return;
	}
	try
	{
		json json = json::parse(r.text);
		if (json["body"]["whoami"] == nullptr) {
			std::println("[BokutachiHook] Missing/Unknown API Key in 'BokutachiAuth.json'");
			Logger("Missing/Unknown API Key in 'BokutachiAuth.json'.");
			AddNotification("Bad API Key for BokutachiIR!");
			return;
		}

		bool permissionsGood = false;
		for (auto& permission : json["body"]["permissions"]) {
			if (permission != "submit_score") continue;
			permissionsGood = true;
			break;
		}
		if (!permissionsGood) {
			std::println("[BokutachiHook] API Key in BokutachiAuth.json is missing 'submit_score' permission");
			Logger("API Key in BokutachiAuth.json is missing 'submit_score' permission.");
			AddNotification("Bad API Key for BokutachiIR!");
			return;
		}
	}
	catch (json::exception& e)
	{
		// what now..?
	}
	AddNotification("BokutachiIR Connected!");
}

void SendScore(const std::string reqBody, bool isDan)
{
	cpr::Response r = cpr::Post(cpr::Url{ isDan? urlDan : url },
								cpr::Header{ {"Content-Type", "application/json"} },
								cpr::Bearer{ apiKey },
								cpr::Body{ reqBody });
	if (r.error.code != cpr::ErrorCode::OK) {
		std::println("[BokutachiHook] Coulnd't POST: {}", r.error.message);
		AddNotification(std::format("Failed to Send Score!\n{}", r.error.message));
	}
	else if (r.status_code != 200) {
		std::println("[BokutachiHook] Score Rejected: {}", r.status_line);
		AddNotification("Score Rejected! Check 'Bokutachi.log'...");
	}
	try
	{
		json log = json::parse(r.text);
		if (log["success"] == false)
		{
			Logger(log["description"]);
			std::println("[BokutachiHook] {}", std::string_view(log["description"]));
		}
	}
	catch (json::exception& e)
	{
		// what now..?
	}
}

std::string FormJSONString(std::string hash) {
	bool hashIsCourse = hash.length() > 32;
	json scorePacket;
	LR2::game& game = *LR2::pGame;
	std::string md5;
	
	if (hashIsCourse) {
		md5 = std::string(hash.begin() + 32, hash.end());
	}
	else md5 = hash;

	scorePacket = {
		{"md5", md5},
		{"playerData", {
					{"autoScr", game.config.play.p1_assist | game.config.play.p2_assist},
					{"gameMode", gameModes[game.config.select.key]},
					{"random", randomModes[game.config.play.random[0]]},
					{"gauge", gauges[game.config.play.gaugeOption[0]]}
					}},
		{"scoreData", {
					{"pgreat", game.gameplay.player[0].judgecount[5]},
					{"great", game.gameplay.player[0].judgecount[4]},
					{"good", game.gameplay.player[0].judgecount[3]},
					{"bad", game.gameplay.player[0].judgecount[2]},
					{"poor", game.gameplay.player[0].judgecount[1]},
					{"maxCombo", hashIsCourse ? game.gameplay.player[0].max_combo_course : game.gameplay.player[0].max_combo},
					{"exScore", game.gameplay.player[0].exscore},
					{"moneyScore", game.gameplay.player[0].score_print},
					{"notesTotal", game.gameplay.player[0].totalnotes},
					{"notesPlayed", game.gameplay.player[0].note_current},
					{"lamp", lamps[game.gameplay.player[0].clearType]},
					{"hpGraph", game.gameplay.statgraph[0].hp}
					}}
	};

	if (!hashIsCourse && game.gameplay.isCourse && game.gameplay.player[0].clearType > 1)
	{
		scorePacket["scoreData"]["lamp"] = lamps[0];
	}
	return scorePacket.dump();
}

typedef int(__cdecl* tUpdateScoreDB)(LR2::CSTR hash, LR2::STATUS* stat, void* sql, LR2::CSTR* passMD5);
tUpdateScoreDB UpdateScoreDB = nullptr;
safetyhook::InlineHook oUpdateScoreDB;
static int __cdecl OnUpdateScoreDB(LR2::CSTR hash, LR2::STATUS* stat, void* sql, LR2::CSTR* passMD5) {
	LR2::game& game = *LR2::pGame;
	std::string message = FormJSONString(hash.body);
	if (game.config.play.is_extra) {
		std::println("[BokutachiHook] Ignoring score to extra mode");
		Logger("Shit!");
		AddNotification("Score not sent - Extra mode enabled");
	}
	else {
		std::println("[BokutachiHook] Trying to send {}", hash.body);
		std::thread(SendScore, std::move(message), std::string_view(hash.body).length() > 32).detach();
	}
	return oUpdateScoreDB.ccall<int>(hash, stat, sql, passMD5);
}

void BokutachiHook::Init() {
	std::println("[BokutachiHook] Initializing.");
	while (!LR2::isInit) Sleep(1);

	moduleBase = (uintptr_t)GetModuleHandle(0);

	try {
		json config;
		{
			std::ifstream conf("BokutachiAuth.json");
			config = json::parse(conf);
		}
		url = config.at("url");
		urlDan = url + "/course";
		apiKey = config.at("apiKey");
	}
	catch (...) {
		std::println("[BokutachiHook] 'BokutachiAuth.json' missing or malformed");
		Logger("'BokutachiAuth.json' is missing or malformed.");
		AddNotification("Missing or malformed 'BokutachiAuth.json'!");
	}
	
	UpdateScoreDB = (tUpdateScoreDB)(moduleBase + 0x45AF0);
	oUpdateScoreDB = safetyhook::create_inline(UpdateScoreDB, OnUpdateScoreDB);

	OnBeforeScreenFlipHk = safetyhook::create_mid(0x4367C6, OnBeforeScreenFlip);

	std::println("[BokutachiHook] Init Done.");
	CheckTachiApi();
}

void BokutachiHook::Deinit() {
	oUpdateScoreDB.~InlineHook();
}