#include <vector>
#include <iostream>
#include <cstring>

#include <BokutachiHook/framework.h>
#include <BokutachiHook/mem.h>

//https://guidedhacking.com/threads/how-to-hack-any-game-first-internal-hack-dll-tutorial.12142/

constexpr unsigned int CALL = 0xE8;
constexpr unsigned int JMP = 0xE9;
constexpr unsigned int NOP = 0x90;
constexpr unsigned int CALL_SIZE = 5;
constexpr unsigned int JMP_SIZE = 5;

bool mem::Detour32(void* src, void* dst, int len)
{
	// Check if len is 5 or greater to make sure the JMP instruction can fit.
	if (len < CALL_SIZE) return false;

	// Setting EXECUTE+READ+WRITE permission for the bytes to alter.
	DWORD curProtection;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

	// Allocating memory for stolen bytes.
	void* gateway = VirtualAlloc(0, len + CALL_SIZE + CALL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (gateway == nullptr)
	{
		std::cout << "Couldn't allocate memory for stolen bytes\n";
		return false;
	}

	// Copying stolen bytes to allocated memory.
	std::memcpy(gateway, src, len);

	// Filling the source with NOPs (needed for len > 5).
	std::memset(src, NOP, len);

	// Calculating relative addresses for JMP from source, CALL from gateway and JMP from gateway.
	uintptr_t relativeAddress = ((uintptr_t)gateway - (uintptr_t)src) - CALL_SIZE;
	intptr_t  gatewayToSourceRelativeAddr = (intptr_t)src - (intptr_t)gateway - CALL_SIZE - JMP_SIZE;
	intptr_t  gatewayToDestinationRelativeAddr = (intptr_t)dst - (intptr_t)gateway - CALL_SIZE - JMP_SIZE;

	// Setting the JMP from source up.
	*(BYTE*)src = JMP;
	*(uintptr_t*)((uintptr_t)src + 1) = relativeAddress;

	// Restoring original page protection for source
	DWORD temp;
	VirtualProtect(src, len, curProtection, &temp);

	// Setting the CALL to destination up.
	*(char*)((uintptr_t)gateway + len) = CALL;
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayToDestinationRelativeAddr;

	// Setting the JMP back from gateway up.
	*(uintptr_t*)((uintptr_t)gateway + len + 5) = JMP;
	*(uintptr_t*)((uintptr_t)gateway + len + 6) = gatewayToSourceRelativeAddr;

	return true;
}

uintptr_t mem::FindDMAAddy(uintptr_t ptr, const std::vector<unsigned int>& offsets)
{
	uintptr_t addr = ptr;
	for (const auto& offset : offsets)
	{
		addr = *(uintptr_t*)addr;
		addr += offset;
	}
	return addr;
}