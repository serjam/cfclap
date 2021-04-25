#pragma once
#include "pch.h"

namespace hax
{
	void PatchIn(BYTE* dst, BYTE* src, unsigned int size);
	void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);

	void WriteNop(BYTE* dst, unsigned int size);
	void WriteNopEx(BYTE* dst, unsigned int size, HANDLE hProcess);

	uintptr_t ResolvePtrChain(uintptr_t base, std::vector<unsigned int> offsets);
	uintptr_t ResolvePtrChainEx(uintptr_t base, std::vector<unsigned int> offsets, HANDLE hProcess);

	bool Detour32In(BYTE* src, BYTE* dst, const uintptr_t len);
	BYTE* TrampHook32In(BYTE* src, BYTE* dst, const uintptr_t len);

	void* TrampHook64(void* pSource, void* pDestination, int dwLen);

}