#include "controlsSender.h"
#include <iostream>

void ControlsSender::setControl(uint8_t control, bool unique)
{
  std::unique_lock<std::mutex> lck(lcMtx);
  if (lastControl != control || !unique)
  {
    lastControl = control;
    cd.notify_all();
  }
}

void ControlsSender::handleEvent(SDL_Event & event)
{
  if (event.type == SDL_KEYDOWN)
  {
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_W:
      setControl(INCREASE, false);
      break;
    case SDL_SCANCODE_S:
      setControl(DECREASE, false);
      break;
    case SDL_SCANCODE_Q:
      setControl(INCREASE_PROP_COEF, false);
      break;
    case SDL_SCANCODE_A:
      setControl(DECREASE_PROP_COEF, false);
      break;
    case SDL_SCANCODE_E:
      setControl(INCREASE_DER_COEF, false);
      break;
    case SDL_SCANCODE_D:
      setControl(DECREASE_DER_COEF, false);
      break;
    case SDL_SCANCODE_SPACE:
      setControl(STOP_MOVING, false);
      break;
    case SDL_SCANCODE_I:
      setControl(INCLINE_FORW, true);
      break;
    case SDL_SCANCODE_K:
      setControl(INCLINE_BACKW, true);
      break;
    case SDL_SCANCODE_J:
      setControl(INCLINE_LEFT, true);
      break;
    case SDL_SCANCODE_L:
      setControl(INCLINE_RIGHT, true);
      break;
    case SDL_SCANCODE_T:
      setControl(INCREASE_PITCH_BIAS, false);
      break;
    case SDL_SCANCODE_G:
      setControl(DECREASE_PITCH_BIAS, false);
      break;
    case SDL_SCANCODE_H:
      setControl(INCREASE_ROLL_BIAS, false);
      break;
    case SDL_SCANCODE_F:
      setControl(DECREASE_ROLL_BIAS, false);
      break;
    }
  }
  if (event.type == SDL_KEYUP)
  {
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_I:
    case SDL_SCANCODE_K:
    case SDL_SCANCODE_J:
    case SDL_SCANCODE_L:
      setControl(INCLINE_DEFAULT, true);
      break;
    }
  }
}

uint8_t * ControlsSender::getControl()
{
  std::unique_lock<std::mutex> lck(mtx);
  cd.wait(lck);
  std::unique_lock<std::mutex> lck2(lcMtx);
  uint8_t * cont = new uint8_t[1];
  *cont = lastControl;
  return cont;
}

void ControlsSender::iterate()
{
  if (!conn.connectIP("192.168.1.5","8081"))
  {
    std::cout << "sending commands not connected, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  while (!shouldStop())
  {
    uint8_t* c = getControl();
    bool sent = conn.sendPacket(c, 1);
    delete [] c;
    if (!sent) return;
  }
}

void ControlsSender::onStop()
{
  cd.notify_all();
}