#define CURL_STATICLIB 1

#include "BokutachiHook.hpp"

#include <iostream>
#include <fstream>
#include <utility>

#include <LR2Mem/LR2Bindings.hpp>
#include <libSafetyhook/safetyhook.hpp>
#include <curl/include/curl/curl.h>
#include <json/single_include/nlohmann/json.hpp>

#pragma comment(lib,"LR2Mem.lib")
#pragma comment(lib,"libSafetyhook.x86.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"wldap32.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"libcrypto.lib")
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcurl.lib")

using json = nlohmann::ordered_json;

static uintptr_t moduleBase = 0;

static std::string url;
static std::string urlDan;
static std::string apiKey;

constexpr const char* lamps[6] = { "NO PLAY", "FAIL", "EASY", "NORMAL", "HARD", "FULL COMBO" };
constexpr const char* gauges[6] = { "GROOVE", "HARD", "HAZARD", "EASY", "P-ATTACK", "G-ATTACK" };
constexpr const char* gameModes[8] = { "ALL", "SINGLE", "7K", "5K", "DOUBLE", "14K", "10K", "9K" };
constexpr const char* randomModes[6] = { "NORAN", "MIRROR", "RAN", "S-RAN", "H-RAN", "ALLSCR" };

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void Logger(std::string buffer)
{
	try
	{
		json log = json::parse(buffer);
		if (log["success"] == false)
		{
			std::ofstream logFile;
			logFile.open("Bokutachi.log", std::ios_base::app);
			auto time = std::chrono::system_clock::now();
			std::time_t timeStamp = std::chrono::system_clock::to_time_t(time);
			logFile << ctime(&timeStamp) << log["description"];
			logFile << std::endl << std::endl;
			logFile.close();
		}
	}
	catch (json::exception& e)
	{
		// what now..?
	}
}

void SendPOST(const std::string reqBody, bool isDan)
{
	CURL* request = curl_easy_init();
	if (request == nullptr)
	{
		std::cout << "Couldn't initialize curl\n";
		return;
	}

	struct curl_slist* headerPOST = nullptr;
	std::string readBuffer;
	headerPOST = curl_slist_append(headerPOST, "Content-Type: application/json");
	headerPOST = curl_slist_append(headerPOST, apiKey.c_str());
	if (!isDan)
		curl_easy_setopt(request, CURLOPT_URL, url.c_str());
	else
		curl_easy_setopt(request, CURLOPT_URL, urlDan.c_str());
	curl_easy_setopt(request, CURLOPT_HTTPHEADER, headerPOST);
	curl_easy_setopt(request, CURLOPT_POSTFIELDS, reqBody.c_str());
	curl_easy_setopt(request, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(request, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(request, CURLOPT_WRITEDATA, &readBuffer);
	curl_easy_setopt(request, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(request, CURLOPT_SSL_VERIFYHOST, 0);

#pragma warning(push)
#pragma warning(disable : 26812)
	CURLcode result = curl_easy_perform(request);
#pragma warning(pop)
	if (result != CURLE_OK)
	{
		std::cout << "Couldn't perform request: " << curl_easy_strerror(result) << std::endl;
	}
	curl_easy_cleanup(request);
	curl_slist_free_all(headerPOST);
	Logger(std::move(readBuffer));
}

std::string FormJSONString(std::string hash) {
	bool hashIsCourse = hash.length() > 32;
	json scorePacket;
	LR2::game& game = *LR2::pGame;
	std::string md5;
	
	if (hashIsCourse) {
		if (game.gameplay.isCourse && game.gameplay.courseType == 2) {
			md5 = std::string(hash.begin() + 32, hash.end());
		}
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
					{"maxCombo", game.gameplay.isCourse ? game.gameplay.player[0].max_combo_course : game.gameplay.player[0].max_combo},
					{"exScore", game.gameplay.player[0].exscore},
					{"moneyScore", game.gameplay.player[0].score_print},
					{"notesTotal", game.gameplay.player[0].totalnotes},
					{"notesPlayed", game.gameplay.player[0].note_current},
					{"lamp", lamps[game.gameplay.player[0].clearType]},
					{"hpGraph", game.gameplay.statgraph[0].hp}
					}}
	};

	if (hashIsCourse && game.gameplay.player[0].clearType > 1)
	{
		scorePacket["scoreData"]["lamp"] = lamps[0];
	}
	return scorePacket.dump();
}

typedef int(__cdecl* tUpdateScoreDB)(LR2::CSTR hash, LR2::STATUS* stat, void* sql, LR2::CSTR* passMD5);
tUpdateScoreDB UpdateScoreDB = nullptr;
safetyhook::InlineHook oUpdateScoreDB;
static int __cdecl OnUpdateScoreDB(LR2::CSTR hash, LR2::STATUS* stat, void* sql, LR2::CSTR* passMD5) {
	std::string message = FormJSONString(hash.body);
	LR2::game& game = *LR2::pGame;
	std::cout << "[BokutachiHook] Trying to send " << hash.body << "\n";
	std::thread(SendPOST, std::move(message), (game.gameplay.isCourse && game.gameplay.courseType == 2)).detach();
	return oUpdateScoreDB.ccall<int>(hash, stat, sql, passMD5);
}

void BokutachiHook::Init() {
	std::cout << "[BokutachiHook] Initializing.\n";
	LR2::Init();
	while (!LR2::isInit) Sleep(1);

	moduleBase = (uintptr_t)GetModuleHandle(0);

	json config;
	{
		std::ifstream conf("BokutachiAuth.json");
		config = json::parse(conf);
	}
	url = config.at("url");
	urlDan = url + "/course";
	apiKey = "Authorization: Bearer ";
	apiKey += config.at("apiKey");

	UpdateScoreDB = (tUpdateScoreDB)(moduleBase + 0x45AF0);
	oUpdateScoreDB = safetyhook::create_inline(UpdateScoreDB, OnUpdateScoreDB);

	std::cout << "[BokutachiHook] Init Done.\n";
}

void BokutachiHook::Deinit() {
	oUpdateScoreDB.~InlineHook();
}