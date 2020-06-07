#pragma once
#include <stdint.h>
#include <condition_variable>
#include <SDL2/SDL.h>
#include "actions.h"
#include "packetsSender.h"

class ControlsSender: public PacketsSender {
private:
  uint8_t lastUniqueControl = 0;
  std::mutex mtx;
  template<typename T>
  void _sendValue(uint8_t key, T value);
public:
  using PacketsSender::PacketsSender;
  void setControl(uint8_t control, bool unique);
  void handleEvent(SDL_Event & event);
  void updateProportionalCoef(float value);
  void sendCommand(uint8_t key);
  void sendValue(uint8_t key, int value);
  void sendValue(uint8_t key, float value);
};
