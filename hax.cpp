#include "pch.h"
#include "hax.h"

void hax::PatchIn(BYTE* dst, BYTE* src, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	memcpy(dst, src, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void hax::PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess)
{
	DWORD oldprotect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	VirtualProtectEx(hProcess, dst, size, oldprotect, &oldprotect);
}

void hax::WriteNop(BYTE* dst, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	memset(dst, 0x90, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void hax::WriteNopEx(BYTE* dst, unsigned int size, HANDLE hProcess)
{
	BYTE* nopArray = new BYTE[size];
	memset(nopArray, 0x90, size);

	PatchEx(dst, nopArray, size, hProcess);
	delete[] nopArray;
}

uintptr_t hax::ResolvePtrChain(uintptr_t base, std::vector<unsigned int> offsets)
{
	uintptr_t addr = base;
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		addr = *(uintptr_t*)addr;
		addr += offsets[i];
	}
	return addr;
}

uintptr_t hax::ResolvePtrChainEx(uintptr_t base, std::vector<unsigned int> offsets, HANDLE hProcess)
{
	uintptr_t addr = base;
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		ReadProcessMemory(hProcess, (BYTE*)addr, &addr, sizeof(addr), 0);
		addr += offsets[i];
	}
	return addr;
}

bool hax::Detour32In(BYTE* src, BYTE* dst, const uintptr_t len)
{
	if (len < 5) return false;

	DWORD oldptc;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldptc);

	uintptr_t relativeAddress = dst - src - 5; // -5 to get RVA between (original+jmp) and new function

	*src = 0xE9; //jmp
	*(uintptr_t*)(src + 1) = relativeAddress; //add our address immediately after jmp

	VirtualProtect(src, len, oldptc, &oldptc);
	return true;
}
BYTE* hax::TrampHook32In(BYTE* src, BYTE* dst, const uintptr_t len)
{
	if (len < 5) return 0;

	/*
	* Create Gateway
	* (what executes stolen bytes and returns back to original fn)
	*/
	BYTE* gateway = (BYTE*)VirtualAlloc(NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	//write the stolen bytes to the gateway
	memcpy_s(gateway, len, src, len);

	//Get the gateway to destination address
	uintptr_t gatewayRelativeAddr = src - gateway - 5;

	// add the jmp opcode to the end of the gateway
	*(gateway + len) = 0xE9;

	//Write the address of the gateway to the jmp
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;

	/*
	* Create Detour
	*/
	Detour32In(src, dst, len);

	// Need this address bc now that we have overwritten
	// original bytes and created a way to resume normal execution,
	// our 'new' function will need to  return to this gateway at the end
	return gateway;
}


void* hax::TrampHook64(void* pSource, void* pDestination, int dwLen)
{
	DWORD MinLen = 14;

	if (dwLen < MinLen) return NULL;

	BYTE stub[] = {
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, //  jmp QWORD PTR [rip+0x0] 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // ptr
	};

	void* pTrampoline = VirtualAlloc(NULL, dwLen + sizeof(stub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	DWORD dwOld = 0;
	VirtualProtect(pSource, dwLen, PAGE_EXECUTE_READWRITE, &dwOld);

	DWORD64 retto = (DWORD64)pSource + dwLen;

	// trampoline
	memcpy(stub + 6, &retto, 8); // copy address of instruction after our jmp from original 
	memcpy((void*)((DWORD_PTR)pTrampoline), pSource, dwLen); // copy stolen bytes into tramp
	memcpy((void*)((DWORD_PTR)pTrampoline + dwLen), stub, sizeof(stub));  // copy return to original 

	// orig
	memcpy(stub + 6, &pDestination, 8); // copy address of our new fn
	memcpy(pSource, stub, sizeof(stub)); // overwrite original with detour 

	for (int i = MinLen; i < dwLen; i++)
	{
		*(BYTE*)((DWORD_PTR)pSource + i) = 0x90;
	}

	VirtualProtect(pSource, dwLen, dwOld, &dwOld);
	return (void*)((DWORD_PTR)pTrampoline);
}