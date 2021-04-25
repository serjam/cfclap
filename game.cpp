#include "pch.h"
#include "game.h"
#include "offsets.h"

void Game::init() {
  Sleep(5000);

  CS_BASE = (DWORD64)GetModuleHandle(L"cshell_x64.dll");
#if DEBUG
  loghex("Base CShell Address is: ", CS_BASE);
#endif

  CF_BASE = (DWORD64)GetModuleHandle(L"crossfire_x64.exe");
#if DEBUG
  loghex("Base CF Address is: ", CF_BASE);
#endif

  CLT_SHELL = *(DWORD64*)(CS_BASE + offs::LT_SHELL);
#if DEBUG
  loghex("CLT_SHELL Address is: ", CLT_SHELL);
#endif

  pLTModel = *(CLTModel**)(CS_BASE + offs::LT_MODELCLIENT_PTR);
#if DEBUG
  loghex("CLTMODEL Address is: ", (DWORD64)pLTModel);
#endif

}

void Game::PrintEntityDbg() {
  if (!CLT_SHELL) return;

  BYTE* curAddr = (BYTE*)(CLT_SHELL + offs::ENT_BEGIN);

  for (int i = 0; i < 16; i++) {
    if (!curAddr) return;
    Player curP = *(Player*)curAddr;

    if (curAddr && *curAddr) {

      printf("PLAYER #%d :: %s :: %d\n", i, curP.info.name, curP.info.team);
    }

    curAddr += offs::ENT_SIZE;
  }
}

bool Game::InGame() {
  return CLT_SHELL && *(BYTE*)(CLT_SHELL + offs::LOCAL_ENT);
}

void Game::GetBonePosition(PlayerModel* obj, uint32_t idx, LTransform* tfm) {
  pLTModel->GetNodeTransform(obj, idx, tfm, true);
}

Player* Game::GetPlayerByIndex(int i) {
  return (Player*)(CLT_SHELL + offs::ENT_BEGIN + (i * offs::ENT_SIZE));
}

void Game::GetAnglesToPlayer(Player* me, Player* target, Vec2& out) {
  LTransform tfmMe, tfmEn;
  GetBonePosition(me->model, 6, &tfmMe);
  GetBonePosition(target->model, 6, &tfmEn);
  Vector3 dV = tfmEn.m_Vec - tfmMe.m_Vec;
  /*
  * Explanation of angles:
  * 
  * For the pitch, (out.y) we calculate the angle using asin,
  * which returns an angle, given the opp & hyp of a right triangle.
  * By passing in the dY from the distance vector as opp and length of 
  * the distance vector as hyp, we can get the angle needed for pitch.
  * (Imagine a triangle where this pitch angle is centered at our eye, the hyp
  * goes from our eye to enemy eye, opp is the delta in Y, and adj is the delta in Z
  * (remember Z is forward/backward in CF). Finally this needs to be negated because in CF
  * aiming all the way up is -90 and all the way down is 90 (reversed of what one might think)
  * 
  * For the yaw, use atan2 which has a range of [-180, 180]. Picture, from birds eye view,
  * you at origin and enemy in first quadrant of plane. Can get angle simply by using the 
  * relative enemy position in x and z. For tan we need opp (use dX) and adj (use dZ).
  * Can also switch them and do 90 - (ans) but thats unnecessarily complicating things
  */
  float dist = dV.Length();
  out.x = atan2(dV.x, dV.z);
  out.y = -asin(dV.y / dist);
}

void Game::aimAtPlayer(Player* me, Player* at) {
  float* yaw = (float*)hax::ResolvePtrChain(CLT_SHELL + offs::LOCAL_ENT, { offs::MP_YAW });
  float* pitch = (float*)hax::ResolvePtrChain(CLT_SHELL + offs::LOCAL_ENT, { offs::MP_PITCH });

  Vec2 newAngles;
  GetAnglesToPlayer(me, at, newAngles);
  *yaw = newAngles.x;
  *pitch = newAngles.y;

}

bool Game::CheckInFOV(float angle, Player* me, Player* target)
{
  Vec2 newAngles;
  GetAnglesToPlayer(me, target, newAngles);

  angle /= 2;
  Vec2 curAngles;
  curAngles.x = *(float*)hax::ResolvePtrChain(CLT_SHELL + offs::LOCAL_ENT, { offs::MP_YAW });
  curAngles.y = *(float*)hax::ResolvePtrChain(CLT_SHELL + offs::LOCAL_ENT, { offs::MP_PITCH });

  // Yaw now needs to be 'normalized' meaning map it from (-inf, +inf) (valid values in the game)
  // to (-180, 180) (what atan2 will return to us)
  // So we have to first use modulo to restrict it to maximum 2pi if its positive and -2pi if its negative
  // Now we need to see:
  // if it's greater than PI, map it to its negative equivalent (-360)
  // if it's less than PI, map it to its positive equivalent (+360)

  if (curAngles.x > 2*M_PI) curAngles.x = std::fmod(curAngles.x, 2*M_PI);
  else if (curAngles.x < -2*M_PI) curAngles.x = std::fmod(curAngles.x, -2*M_PI);
  if (curAngles.x > M_PI) curAngles.x -= 2 * M_PI;
  else if (curAngles.x < -M_PI) curAngles.x += 2 * M_PI;

  return (newAngles.y >= (curAngles.y - angle) &&
    newAngles.y <= (curAngles.y + angle) &&
    newAngles.x >= (curAngles.x - angle) &&
    newAngles.x <= (curAngles.x + angle));
}

void Game::FOVAimbot() {
  while (GetAsyncKeyState(VK_LSHIFT)) {
    if (!CLT_SHELL || !InGame()) return;

    BYTE myIdx = *(BYTE*)(CLT_SHELL + offs::LOCAL_ENT_INDEX);
    Player* me = GetPlayerByIndex(myIdx);
    Player* target = nullptr;

    for (int i = 0; i < 16; i++) {
      Player* enem = GetPlayerByIndex(i);
      if (CheckInFOV(0.10, me, enem) && enem->info.team != me->info.team 
                                     && enem->info.health != 0) {
        target = enem;
        break;
      }
    }

    while (GetAsyncKeyState(VK_LSHIFT) && target 
                                       && target->info.health) {
      aimAtPlayer(me, target);
    }

  }
}
