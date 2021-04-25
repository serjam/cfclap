#include "pch.h"
#include "DX.h"
#include "hax.h"
#include "utils.h"
#include "game.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

tEndScene DX::gtEndScene = nullptr;
WNDPROC DX::oWndProc = nullptr;

DX::DX() {
  menuInit = false;
  menuShow = true;
  DXLINE = nullptr;
  DXFONT = nullptr;
  DXVB = nullptr;
  m_pDevice = nullptr;
}
bool DX::GetD3D9VT(void** pTable, size_t size) {
  if (!pTable)
    return false;

  IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

  if (!pD3D)
    return false;

  IDirect3DDevice9* pDummyDevice = nullptr;

  D3DPRESENT_PARAMETERS d3dpp = {};
  d3dpp.Windowed = true;
  d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  d3dpp.hDeviceWindow = GetForegroundWindow();

  HRESULT dummyDevCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

  if (dummyDevCreated != S_OK) {
#if DEBUG
    log("ERROR: Couldn't get VTable...");
#endif
    return false;
  }

  memcpy(pTable, *(void***)(pDummyDevice), size);

#if DEBUG
  printf("EndScene Address :: %p\n", pTable[42]);
  printf("vTable Address :: %p\n", pTable);
#endif

  pDummyDevice->Release();
  pD3D->Release();
  return true;
}
void DX::Hook() {
  bool getdev = GetD3D9VT(vTable, sizeof(vTable));
  if (getdev) {
    // Found out 15 by looking at memory and finding first integer >=14 that was hookable (not breaking across instructions)
    memcpy(oMemEndScene, vTable[42], 15);
    gtEndScene = (tEndScene)hax::TrampHook64((BYTE*)vTable[42], (BYTE*)DX::hkEndScene, 15);

#if DEBUG
    log("Successful D3D Hook...");
#endif

    HWND window = FindWindowA(NULL, "CROSSFIRE");
    oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)DX::hkWndProc);
#if DEBUG
    log("Successful Window Hook...");
#endif


  }
  else {
#if DEBUG
    log("Failed D3D Hook...");
#endif
  }

}
void DX::Unhook() {
  DWORD oldProtect = 0x0;
  DWORD tmp = 0x0;
  VirtualProtect(vTable[42], 15, PAGE_EXECUTE_READWRITE, &oldProtect);
  memcpy(vTable[42], oMemEndScene, 15);
  VirtualProtect(vTable[42], 15, oldProtect, &tmp);
  VirtualFree(gtEndScene, 0, MEM_RELEASE); //Dangerous? hkEndScene still might be returning this..
  
  if (menuInit) {
    CleanMenu();
    SetWindowLongPtr(FindWindowA(NULL, "CROSSFIRE"), GWLP_WNDPROC, (LONG_PTR)DX::oWndProc);
  }
}
void DX::InitMenu() {
  DX& g_DX = DX::get();

  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui_ImplWin32_Init(FindWindowA(NULL, "CROSSFIRE"));
  ImGui_ImplDX9_Init(g_DX.m_pDevice);

  menuInit = true;
}
void DX::CleanMenu() {
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}
void DX::RenderMenu() {
  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("CFCLAP v1.0");
  ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
  ImGui::Checkbox("Enable Aimbot", &aimbotOn);
  ImGui::Checkbox("Enable ESP", &espOn);

  ImGui::SliderFloat("Crosshair Size", &xhrSize, 0.0f, 1.0f);
  ImGui::ColorEdit3("Crosshair Color", (float*)&xhrColor);
  ImGui::End();

  ImGui::EndFrame();
  ImGui::Render();
  ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

HRESULT __stdcall DX::hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
  DX& g_DX = DX::get();

  if (pDevice != g_DX.m_pDevice) {
#if DEBUG
    log("REPLACE DEVICE");
#endif
    if (g_DX.menuInit) g_DX.CleanMenu();
    if (g_DX.DXFONT) g_DX.DXFONT->Release();
    if (g_DX.DXLINE) g_DX.DXLINE->Release();
    g_DX.DXFONT = nullptr;
    g_DX.DXLINE = nullptr;

    g_DX.m_pDevice = pDevice;
    g_DX.InitMenu();
  }

  if (g_DX.m_pDevice) {
    if (g_DX.menuShow)
      g_DX.RenderMenu();

    g_DX.DrawESP2D();
    g_DX.DrawCircle(400.0, 300.0, g_DX.xhrSize*100.0, 0, true, 32, 
      D3DCOLOR_RGBA(
        (int)(g_DX.xhrColor.x * 255.0f), 
        (int)(g_DX.xhrColor.y * 255.0f), 
        (int)(g_DX.xhrColor.z * 255.0f), 
        150)
      );
  }

  return gtEndScene(pDevice);
}

LRESULT __stdcall DX::hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  DX& g_DX = DX::get();

  if (g_DX.menuShow) {
    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
  }

  return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void DX::DrawCircle(float x, float y, float radius, int rotate, bool smoothing, int resolution, DWORD color)
{

  std::vector<vertex> circle(resolution + 2);
  float angle = rotate * D3DX_PI / 180;
  float pi = D3DX_PI;


  for (int i = 0; i < resolution + 2; i++)
  {
    circle[i].x = (float)(x - radius * cos(i * (2 * pi / resolution)));
    circle[i].y = (float)(y - radius * sin(i * (2 * pi / resolution)));
    circle[i].z = 0;
    circle[i].rhw = 1;
    circle[i].color = color;
  }

  // Rotate matrix
  int _res = resolution + 2;
  for (int i = 0; i < _res; i++)
  {
    circle[i].x = x + cos(angle) * (circle[i].x - x) - sin(angle) * (circle[i].y - y);
    circle[i].y = y + sin(angle) * (circle[i].x - x) + cos(angle) * (circle[i].y - y);
  }

  m_pDevice->CreateVertexBuffer((resolution + 2) * sizeof(vertex), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &DXVB, NULL);

  VOID* pVertices;
  DXVB->Lock(0, (resolution + 2) * sizeof(vertex), (void**)&pVertices, 0);
  memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(vertex));
  DXVB->Unlock();


  m_pDevice->SetTexture(0, NULL);
  m_pDevice->SetPixelShader(NULL);
  if (smoothing)
  {
    m_pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
  }
  m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  m_pDevice->SetStreamSource(0, DXVB, 0, sizeof(vertex));
  m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

  m_pDevice->DrawPrimitive(D3DPT_LINESTRIP, 0, resolution);
  if (DXVB != NULL) DXVB->Release();
}
void DX::HXDrawText(const char* text, float x, float y, D3DCOLOR color) {
  RECT rect;

  if (!DXFONT)
    D3DXCreateFont(m_pDevice, 14, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial", &DXFONT);

  SetRect(&rect, x + 1, y + 1, x + 1, y + 1);
  DXFONT->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));

  SetRect(&rect, x, y, x, y);
  DXFONT->DrawTextA(NULL, text, -1, &rect, DT_CENTER | DT_NOCLIP, color);
}
void DX::DrawLine(vec2 start, vec2 end, int thickness, D3DCOLOR color) {
  if (!DXLINE)
    D3DXCreateLine(m_pDevice, &DXLINE);

  D3DXVECTOR2 Line[2];
  Line[0] = D3DXVECTOR2(start.x, start.y);
  Line[1] = D3DXVECTOR2(end.x, end.y);
  DXLINE->SetWidth(thickness);
  DXLINE->Draw(Line, 2, color);

}
void DX::DrawBox2D(vec2 top, vec2 bot, int thickness, D3DCOLOR color) {
  int height = abs(top.y - bot.y);
  vec2 tl, tr;
  tl.x = top.x - height / 4;
  tr.x = top.x + height / 4;
  tl.y = tr.y = top.y;

  vec2 bl, br;
  bl.x = bot.x - height / 4;
  br.x = bot.x + height / 4;
  bl.y = br.y = bot.y;

  DrawLine(tl, tr, thickness, color);
  DrawLine(bl, br, thickness, color);
  DrawLine(tl, bl, thickness, color);
  DrawLine(tr, br, thickness, color);
}

bool DX::w2s(D3DXVECTOR3* InOut) {
  D3DXVECTOR3 vScreen;
  D3DXVECTOR3 PlayerPos(InOut->x, InOut->y, InOut->z);
  D3DVIEWPORT9 viewPort = { 0 };
  D3DXMATRIX view, projection, world;
  m_pDevice->GetTransform(D3DTS_VIEW, &view);
  m_pDevice->GetTransform(D3DTS_PROJECTION, &projection);
  m_pDevice->GetTransform(D3DTS_WORLD, &world);
  m_pDevice->GetViewport(&viewPort);
  D3DXVec3Project(&vScreen, &PlayerPos, &viewPort, &projection, &view, &world);
  if (vScreen.z < 1.0f && vScreen.x > 0.0f && vScreen.y > 0.0f && vScreen.x < viewPort.Width && vScreen.y < viewPort.Height)
  {
    *InOut = vScreen;
    return true;
  }
  return false;
}

void DX::DrawESP2D() {
  Game& g_CF = Game::get();

  if (!g_CF.CLT_SHELL || !g_CF.InGame() || !espOn) return;

  BYTE* curAddr = (BYTE*)(g_CF.CLT_SHELL + offs::ENT_BEGIN);

  // Loop Entity List
  for (int i = 0; i < 16; i++) {
    Player curP = *(Player*)curAddr;

    if (curP.info.health) {
      D3DXVECTOR3 ptH = D3DXVECTOR3(curP.model->posH.x, curP.model->posH.y, curP.model->posH.z);
      D3DXVECTOR3 ptF = D3DXVECTOR3(curP.model->posF.x, curP.model->posF.y, curP.model->posF.z);
      if (w2s(&ptH) && w2s(&ptF)) {

        vec2 top, bot;
        top.x = ptH.x;
        top.y = ptH.y;
        bot.x = ptF.x;
        bot.y = ptF.y;

        if (curP.info.team == 1)
          DrawBox2D(top, bot, 2, D3DCOLOR_ARGB(255, 0, 0, 255));
        else
          DrawBox2D(top, bot, 2, D3DCOLOR_ARGB(255, 220, 170, 40));

        HXDrawText(curP.info.name, bot.x, bot.y - 12, D3DCOLOR_ARGB(255, 255, 255, 255));
        HXDrawText(std::to_string(curP.info.health).c_str(), top.x, top.y + 4, D3DCOLOR_ARGB(255, 255, 255, 255));

      }
    }

    curAddr += offs::ENT_SIZE;
  }

}