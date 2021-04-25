#pragma once
namespace offs {
  // To get signatures, check what accesses the addresses (not the pointers)
  // of the values found, should find instructions where the offsets for 
  // static pointers matches with the static pointer found earlier in CE

  //48 8B ? ? ? ? ? 48 8B ? FF 90 ? ? ? ? 40 3A
  //mov rcx, [cshell_x64.dll + 1BE4890]
  const DWORD LT_SHELL = 0x1BE4890;
  //48 8B ? ? ? ? ? 48 8B ? 44 0F ? ? 44 8B ? ? ? 48 8B ? ? FF 50 ? 44 89
  //mov rcx,[cshell_x64.dll+1BE1470]
  const DWORD LT_MODELCLIENT_PTR = 0x1BE1470;
  //48 8B ? ? ? ? ? 48 8B ? FF 90 ? ? ? ? 48 8B ? 48 8B ? FF 92 ? ? ? ? 48 3B ? 74
  //mov rcx,[rax+00000090]
  const DWORD LOCAL_ENT = 0x90;
  //48 69 C8 ? ? ? ? 48 8B ? ? ? ? ? 4C 8B ? ? ? ? ? ? 48 85
  //imul rcx, rax, 00000D98
  const DWORD ENT_SIZE = 0xD98;
  //4C 8B ? ? ? ? ? ? 48 85 ? 75 ? 8D 4E
  //mov r15, [rcx + rdi + 00000290]
  const DWORD ENT_BEGIN = 0x290; 
  //0F B6 ? ? ? ? ? 41 0F ? ? ? 48 8B
  //movzx ecx, byte ptr[rsi + 00000288]
  const DWORD LOCAL_ENT_INDEX = 0x288; 

  const DWORD BOT_PITCH = 0xE8C;
  const DWORD BOT_YAW = 0xE90;
  const DWORD MP_PITCH = 0x5DC;
  const DWORD MP_YAW = 0x5E0;

}
