#include "pch.h"
#include "utils.h"
#include "game.h"
#include "hax.h"
#include "DX.h"

DWORD WINAPI hxT(HMODULE hModule) {

#if DEBUG
  AllocConsole();
  FILE* f;
  freopen_s(&f, "CONOUT$", "w", stdout);
  log("Debug Mode Enabled");
#endif

  DX& g_DX = DX::get();
  Game& g_CF = Game::get();
  g_CF.init();
  g_DX.Hook();

  while (1) {
    // CAPS - Toggle Menu
    if (GetAsyncKeyState(VK_CAPITAL) & 1) {
      g_DX.menuShow = !g_DX.menuShow;
    }
    // DEL - Print Entity List
#if DEBUG
    if (GetAsyncKeyState(VK_DELETE) & 1) {
      log("Call to PrintEntityDbg");
      g_CF.PrintEntityDbg();
    }
#endif
    // PageDOWN - Unhook/Detach
    if (GetAsyncKeyState(VK_NEXT) & 1) {
      g_DX.Unhook();
      break;
    }
    // Left Shift - Aimbot 
    if (GetAsyncKeyState(VK_LSHIFT) && g_DX.aimbotOn) {
      g_CF.FOVAimbot();
    }
  }

#if DEBUG
  log("Exiting...");  
  fclose(f);
  FreeConsole();
#endif

  FreeLibraryAndExitThread(hModule, 0);

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
    CloseHandle(
      CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)hxT, hModule, 0, nullptr)
    );
    DisableThreadLibraryCalls(hModule);
  }
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}


