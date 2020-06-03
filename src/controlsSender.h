#pragma once
#include <stdint.h>
#include <condition_variable>
#include <SDL2/SDL.h>
#include "constants.h"
#include "separateLoop.h"
#include "connection.h"

class ControlsSender: public SeparateLoop {
private:
  Connection conn;
  uint8_t lastControl = 0;
  std::mutex lcMtx;
  std::mutex mtx;
  std::condition_variable cd;
  uint8_t* getControl();
  void iterate();
  void onStop();
public:
  void setControl(uint8_t control, bool unique);
  void handleEvent(SDL_Event & event);
};
