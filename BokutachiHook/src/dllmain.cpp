#include <curl/curl.h>

#include <fstream>
#include <iostream>
#include <thread>
#include <cstdint>
#include <utility>

#include <BokutachiHook/framework.h>
#include <BokutachiHook/mem.h>
#include <BokutachiHook/JSON.h>

//#define BOKUTACHI_CONSOLE_ENABLED


constexpr unsigned int SCORE_OFFSET = 0x6B1548;

uintptr_t moduleBase = (uintptr_t)GetModuleHandle("LR2body.exe");
uintptr_t scoreAddr = mem::FindDMAAddy(moduleBase + SCORE_OFFSET, { 0x4 });
uintptr_t playerAddr = 0xEF784;

struct scoreStruct
{
	std::int32_t head;
	std::int32_t unknown1[20];
	std::int32_t poor;
	std::int32_t bad;
	std::int32_t good;
	std::int32_t great;
	std::int32_t pgreat;
	std::int32_t combo;
	std::int32_t unknown2[11]; //I haven't studied this struct segment yet.
	std::int32_t notestotal;
	std::int32_t notesplayed;
	std::int32_t unknown3[33]; //And this :P.
	std::int32_t exscore;
	std::int32_t unknown4[4]; //You get it.
	std::int32_t moneyScore;
	std::int32_t unknown5[4];
	std::int32_t lamp;
	std::int32_t unknown6[82];
	std::int32_t hpgraph[1000];
};
constexpr unsigned int SCORE_STRUCT_SIZE = 4664;
static_assert(sizeof(scoreStruct) == SCORE_STRUCT_SIZE, "Incorrect score struct size");

struct playerStruct
{
	std::int32_t head;
	std::int32_t unknown1[44];
	std::int32_t hiSpeed;
	std::int32_t unknown3;
	std::int32_t gauge;
	std::int32_t unknown4;
	std::int32_t random;
	std::int32_t unknown5;
	std::int32_t laneCover;
	std::int32_t unknown6[3];
	std::int32_t sudPlus1;
	std::int32_t sudPlus2;
	std::int32_t unknown7[2];
	std::int32_t autoScr;
	std::int32_t unknown8[3];
	std::int32_t hiSpeedType;
	std::int32_t battle;
	std::int32_t unknown9[15];
	std::int32_t hiSpeedMax;
	std::int32_t unknown[50];
	std::int32_t gameMode;
};
constexpr unsigned int PLAYER_STRUCT_SIZE = 528;
static_assert(sizeof(playerStruct) == PLAYER_STRUCT_SIZE, "Incorrect player struct size");

char md5[33];
constexpr unsigned int MD5_SIZE = 32;
constexpr const char* lamps[6] = { "NO PLAY", "FAIL", "EASY", "NORMAL", "HARD", "FULL COMBO" };
constexpr const char* gauges[6] = { "GROOVE", "HARD", "HAZARD", "EASY", "P-ATTACK", "G-ATTACK" };
constexpr const char* gameModes[8] = { "ALL", "SINGLE", "7K", "5K", "DOUBLE", "14K", "10K", "9K" };
constexpr const char* randomModes[6] = { "NORAN", "MIRROR", "RAN", "S-RAN", "H-RAN", "ALLSCR" };
std::string url;
std::string apiKey;

void SendPOST(const std::string reqBody)
{

	CURL* request = curl_easy_init();
	if (request == nullptr)
	{
		std::cout << "Couldn't initialize curl\n";
		return;
	}

	struct curl_slist* headerPOST = nullptr;
	headerPOST = curl_slist_append(headerPOST, "Content-Type: application/json");
	headerPOST = curl_slist_append(headerPOST, apiKey.c_str());
	curl_easy_setopt(request, CURLOPT_URL, url.c_str());
	curl_easy_setopt(request, CURLOPT_HTTPHEADER, headerPOST);
	curl_easy_setopt(request, CURLOPT_POSTFIELDS, reqBody.c_str());
	curl_easy_setopt(request, CURLOPT_CUSTOMREQUEST, "POST");

#pragma warning(push)
#pragma warning(disable : 26812)
	CURLcode response = curl_easy_perform(request);
#pragma warning(pop)
	if (response != CURLE_OK)
	{
		std::cout << "Couldn't perform request\n";
	}
	curl_easy_cleanup(request);
	curl_slist_free_all(headerPOST);
}

void FormJSON(const scoreStruct scoreData, const playerStruct playerData, char md5[])
{
	json scorePacket;
	scorePacket = {
		{"md5", md5},
		{"playerData", {
					{"autoScr", playerData.autoScr},
					{"gameMode", gameModes[playerData.gameMode]},
					{"random", randomModes[playerData.random]},
					{"gauge", gauges[playerData.gauge]}
					}},
		{"scoreData", {
					{"pgreat", scoreData.pgreat},
					{"great", scoreData.great},
					{"good", scoreData.good},
					{"bad", scoreData.bad},
					{"poor", scoreData.poor},
					{"maxCombo", scoreData.combo},
					{"exScore", scoreData.exscore},
					{"moneyScore", scoreData.moneyScore},
					{"notesTotal", scoreData.notestotal},
					{"notesPlayed", scoreData.notesplayed},
					{"lamp", lamps[scoreData.lamp]},
					{"hpGraph", scoreData.hpgraph}
					}}
	};

	std::string reqBody = scorePacket.dump();
	SendPOST(std::move(reqBody));
}


void DumpData()
{
	uintptr_t md5Addr = mem::FindDMAAddy(moduleBase + 0x2C05C, { 0x18, 0x8, 0x0 });
	memcpy(md5, (int*)md5Addr, MD5_SIZE);
	md5[32] = '\0';

	scoreStruct scoreData;
	memcpy(&scoreData, (int*)scoreAddr, sizeof(scoreData));
	playerStruct playerData;
	memcpy(&playerData, (int*)playerAddr, sizeof(playerData));

	FormJSON(std::move(scoreData), std::move(playerData), md5);
}

void ThreadStarter()
{
	std::thread dumpThread(DumpData);
	dumpThread.detach();
}


DWORD WINAPI HackThread(HMODULE hModule)
{
#ifdef BOKUTACHI_CONSOLE_ENABLED
	AllocConsole();
	FILE* f = nullptr;
	freopen_s(&f, "CONOUT$", "w", stdout);

	std::cout << "OG for a fee, stay sippin' fam\n";
#endif
	json config;
	{
		std::ifstream conf("BokutachiAuth.json");
		config = json::parse(conf);
	}
	url = config.at("url");
	apiKey = "Authorization: Bearer ";
	apiKey += config.at("apiKey");
	mem::Detour32((void*)(moduleBase + 0x45C17), (void*)&ThreadStarter, 5); // 0x203EB send with LR2IR, replaced 6 bytes


#ifdef BOKUTACHI_CONSOLE_ENABLED
	while (true)
	{
	}
	fclose(f);
	FreeConsole();
#endif

#pragma warning(push)
#pragma warning(disable : 6258)
	TerminateThread(GetCurrentThread(), 0);
#pragma warning(pop)
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{

		auto* hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr);
		if (hThread == nullptr)
		{
			std::cout << "Couldn't create main thread\n";
			return FALSE;
		}

		CloseHandle(hThread);
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}