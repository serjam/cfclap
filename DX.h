#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma once

typedef HRESULT(__stdcall* tEndScene)(LPDIRECT3DDEVICE9 pDevice);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

class DX
{
private:
  DX();

  struct vec2 { float x; float y; };
  struct vec3 { float x; float y; float z; };
  struct vertex
  {
    FLOAT x, y, z, rhw;
    DWORD color;
  };
  static HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice);
  static LRESULT __stdcall hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  static tEndScene gtEndScene;
  static WNDPROC oWndProc;

  BYTE oMemEndScene[15];
  IDirect3DVertexBuffer9* DXVB;
  void* vTable[119];

  bool GetD3D9VT(void** pTable, size_t size);
  void DrawCircle(float x, float y, float radius, int rotate, bool smoothing, int resolution, DWORD color);
  void HXDrawText(const char* text, float x, float y, D3DCOLOR color);
  void DrawLine(vec2 start, vec2 end, int thickness, D3DCOLOR color);
  void DrawBox2D(vec2 top, vec2 bot, int thickness, D3DCOLOR color);

public:
  ID3DXLine* DXLINE;
  ID3DXFont* DXFONT;
  LPDIRECT3DDEVICE9 m_pDevice;

  bool menuInit;
  bool menuShow;
  bool aimbotOn;
  bool espOn;
  float xhrSize;
  vec3 xhrColor;

  DX(DX const&) = delete;
  DX& operator=(DX const&) = delete;

  static DX& get() {
    static DX inst;
    return inst;
  }

  void Hook();
  void Unhook();
  void InitMenu();
  void RenderMenu();
  void CleanMenu();
  bool w2s(D3DXVECTOR3* InOut);
  void DrawESP2D();
};




