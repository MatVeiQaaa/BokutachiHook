#include <curl/curl.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>
#include <chrono>
#include <ctime>

#include <BokutachiHook/JSON.h>
#include <BokutachiHook/framework.h>
#include <BokutachiHook/mem.h>
#include <BokutachiHook/winver.h>

constexpr unsigned int SCORE_OFFSET = 0x6B1548;

uintptr_t moduleBase;
uintptr_t scoreAddr = 0x18715C;
uintptr_t playerAddr = 0xEF784;

double winver = 0;
unsigned int win10Offset = 0;

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
	std::int32_t unknown6[69];
	std::int32_t isComplete;
	std::int32_t unknown7[12];
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
char md5Dan[129];
constexpr unsigned int MD5_SIZE = 32;
constexpr unsigned int MD5DAN_SIZE = 128;
int lamp;
constexpr const char* lamps[6] = { "NO PLAY", "FAIL", "EASY", "NORMAL", "HARD", "FULL COMBO" };
constexpr const char* gauges[6] = { "GROOVE", "HARD", "HAZARD", "EASY", "P-ATTACK", "G-ATTACK" };
constexpr const char* gameModes[8] = { "ALL", "SINGLE", "7K", "5K", "DOUBLE", "14K", "10K", "9K" };
constexpr const char* randomModes[6] = { "NORAN", "MIRROR", "RAN", "S-RAN", "H-RAN", "ALLSCR" };
bool isDan;
std::string url;
std::string urlDan;
std::string apiKey;

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
	catch (json::exception &ex)
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

#pragma warning(push)
#pragma warning(disable : 26812)
	CURLcode result = curl_easy_perform(request);
#pragma warning(pop)
	if (result != CURLE_OK)
	{
		std::cout << "Couldn't perform request\n";
	}
	curl_easy_cleanup(request);
	curl_slist_free_all(headerPOST);
	Logger(std::move(readBuffer));
	std::cout << "SendPOST exit" << std::endl;
}

void FormJSON(const scoreStruct scoreData, const playerStruct playerData, char md5[], bool isDan)
{
	std::cout << "in FormJSON" << std::endl;
	std::cout << md5 << std::endl;
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

	if (scoreData.isComplete == 0)
	{
		scorePacket["scoreData"]["lamp"] = lamps[0];
	}
	std::string reqBody = scorePacket.dump();
	std::cout << "SendPOST call\n";
	SendPOST(std::move(reqBody), isDan);
	std::cout << "FormJSON exit\n";
}


void DumpData()
{
	std::cout << "DumpData start\n";
	scoreStruct scoreData;
	memcpy(&scoreData, (int*)(scoreAddr + win10Offset), sizeof(scoreData));
	std::cout << "memcpy scoreData done\n";
	playerStruct playerData;
	memcpy(&playerData, (int*)(playerAddr + win10Offset), sizeof(playerData));
	std::cout << "memcpy playerData done\n";
	std::cout << "FormJSON call\n";
	if (!isDan)
		FormJSON(std::move(scoreData), std::move(playerData), md5, isDan);
	else
		FormJSON(std::move(scoreData), std::move(playerData), md5Dan, isDan);
	std::cout << "DumpData exit\n";
}

void ThreadStarter()
{
	uintptr_t md5Addr;

#ifdef _DEBUG
	__asm {
		PUSH EAX
		PUSH EBX
		MOV EAX, ESP
		ADD EAX, 0x74 // You bet this will be wrong if built with different compiler.
		MOV EBX, [EAX]
		MOV md5Addr, EBX
		POP EBX
		POP EAX
};
 // DEBUG
#else
	__asm {
		PUSH EAX
		PUSH EBX
		MOV EAX, ESP
		ADD EAX, 0x6C // You bet this will be wrong if built with different compiler.
		MOV EBX, [EAX]
		MOV md5Addr, EBX
		POP EBX
		POP EAX
	};
#endif

	std::cout << "memcpy md5\n";
	memcpy(md5, (int*)md5Addr, MD5_SIZE);
	std::cout << "memcpy md5 done\n";
	md5[32] = '\0';
	isDan = false;
	if (strncmp(md5, "0000000000200000000000000000????", 16) == 0) // The "????" would be different depending on what dan is played (stella/satellite/genoside).
	{
		std::cout << "md5 is dan" << std::endl;
		memcpy(md5Dan, (int*)md5Addr + 8, MD5DAN_SIZE);
		md5Dan[128] = '\0';
		isDan = true;
	}

	std::thread dumpData(DumpData);
	dumpData.detach();
}

void PatchPreviewMemleak()
{
	WORD* patchAddr = (WORD*)(moduleBase + 0xB0822);
	DWORD curProtection;
	VirtualProtect(patchAddr, sizeof(WORD), PAGE_EXECUTE_READWRITE, &curProtection);

	*patchAddr = (WORD)0x16EB;

	DWORD temp;
	VirtualProtect(patchAddr, sizeof(WORD), curProtection, &temp);
}

DWORD WINAPI HackThread(HMODULE hModule)
{
#ifdef _DEBUG
	AllocConsole();
	FILE* f = nullptr;
	freopen_s(&f, "CONOUT$", "w", stdout);

	std::cout << "OG for a fee, stay sippin' fam\n";
#endif
	if ((moduleBase = (uintptr_t)GetModuleHandle("LR2body.exe")) == 0)
	{
		moduleBase = (uintptr_t)GetModuleHandle("LRHbody.exe");
	}

	winver = getSysOpType();
	//winver = 10;
	if (winver >= 10)
	{
		win10Offset = 0x10000;
	}
	std::cout << "winver: " << winver << std::endl;
	std::cout << "win10Offset: " << win10Offset << std::endl;
	json config;
	{
		std::ifstream conf("BokutachiAuth.json");
		config = json::parse(conf);
	}
	url = config.at("url");
	urlDan = url + "/course";
	apiKey = "Authorization: Bearer ";
	apiKey += config.at("apiKey");
	PatchPreviewMemleak();
	mem::Detour32((void*)(moduleBase + 0x45C1C), (void*)&ThreadStarter, 6);
	std::cout << "Detour32 executed\n";


#ifdef _DEBUG
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
