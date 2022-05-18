#pragma once

#include <vector>

#include <BokutachiHook/framework.h>

//https://guidedhacking.com/threads/how-to-hack-any-game-first-internal-hack-dll-tutorial.12142/

namespace mem
{
	bool Detour32(void* src, void* dst, int len);
	uintptr_t FindDMAAddy(uintptr_t ptr, const std::vector<unsigned int>& offsets);
}