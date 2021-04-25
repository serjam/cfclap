#include "geom.h"
#include "utils.h"
#include "offsets.h"
#include "hax.h"
#pragma once

/*      Game Structs        */
struct PlayerModel {
  char unk0[8]; //vTable
  // X: left-/right+ 
  // Y: vertical down-/up+
  // Z: back-/forward+
  Vector3 posF;
  Vector3 posH;
};
struct PlayerInfo {
  __int8 clientID;
  __int8 team;
  char name[12];
  char unk0[46];
  __int32 health;
};
struct Player {
  PlayerModel* model;
  PlayerInfo info;
};
struct LTransform {
  Vector3 m_Vec;
  char unk[2048];
};


template< class type > type GetVFn(PVOID base, SIZE_T index)
{
  DWORD64* vtablefunc = *(DWORD64**)base;
  return (type)(vtablefunc[index]);
}

class CLTModel
{
public:
  uint32_t GetNodeTransform(PlayerModel* hObj, uint32_t hNode, LTransform* transform, bool bWorldSpace)
  {
    typedef uint32_t(__thiscall* oGetNodeTransform)(void*, PlayerModel*, uint32_t, LTransform*, bool);
    return GetVFn<oGetNodeTransform>(this, 15)(this, hObj, hNode, transform, bWorldSpace);
  }
};

class Game {
private:
  Game() {}

public:
  DWORD64 CS_BASE = 0;
  DWORD64 CF_BASE = 0;
  DWORD64 CLT_SHELL = 0;

  CLTModel* pLTModel = 0;

  Game(Game const&) = delete;
  Game& operator=(Game const&) = delete;

  static Game& get() {
    static Game cf;
    return cf;
  }
  
  void init();
  void PrintEntityDbg();
  bool InGame();

  void GetBonePosition(PlayerModel* obj, uint32_t idx, LTransform* tfm);

  Player* GetPlayerByIndex(int i);
  void GetAnglesToPlayer(Player* me, Player* target, Vec2& out);

  void aimAtPlayer(Player* me, Player* at);
  bool CheckInFOV(float angle, Player* me, Player* target);
  void FOVAimbot();
};
