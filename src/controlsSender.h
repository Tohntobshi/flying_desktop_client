#pragma once
#include <stdint.h>
#include <condition_variable>
#include <SDL2/SDL.h>
#include "actions.h"
#include "packetsSender.h"

class ControlsSender: public PacketsSender {
  uint8_t lastUniqueControl = 0;
  std::mutex mtx;
public:
  using PacketsSender::PacketsSender;
  void setControl(uint8_t control, bool unique);
  void handleEvent(SDL_Event & event);
  void updateProportionalCoef(float value);
  void sendCommand(uint8_t key);
  template<typename T>
  void sendValue(uint8_t key, T value);
};
