#include "Version.hpp"

#include <cpr/cpr.h>
#include <json/single_include/nlohmann/json.hpp>
#include <PicoSHA2/picosha2.h>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")

static std::string hashToStr(std::array<unsigned char, picosha2::k_digest_size>& hash) {
    std::ostringstream os;
    os.setf(std::ios::hex, std::ios::basefield);
    for (auto& it : hash) {
        os.width(2);
        os.fill('0');
        os << static_cast<unsigned int>(it);
    }
    os.setf(std::ios::dec, std::ios::basefield);
    return os.str();
}

std::optional<bool> Version::Check() {
    cpr::Response r = cpr::Get(cpr::Url{ "https://api.github.com/repos/MatVeiQaaa/BokutachiHook/releases" });

    if (r.status_code != 200) return std::nullopt;
    nlohmann::json json = nlohmann::json::parse(r.text);
    std::string shaRemote;
    if (json.size()) {
        auto& lastRelease = json[0];
        for (auto& asset : lastRelease["assets"]) {
            if (asset["name"] != "BokutachiHook.dll") continue;
            shaRemote = asset["digest"];
            break;
        }
    }
    if (shaRemote.empty()) return std::nullopt;
    shaRemote = shaRemote.substr(7);
    std::ifstream f("BokutachiHook.dll", std::ios::binary);
    std::array<unsigned char, picosha2::k_digest_size> shaLocalBin;
    picosha2::hash256(f, shaLocalBin.begin(), shaLocalBin.end());
    std::string shaLocal = hashToStr(shaLocalBin);
    return shaLocal == shaRemote;
}