#pragma once
#include <stdint.h>
#include "separateLoop.h"
#include "packetsReceiver.h"
#include "frameCapturer.h"

class FrameDecoder: public SeparateLoop {
private:
  PacketsReceiver * packetsReceiver = nullptr;
  FrameCapturer * frameCapt = nullptr;
  void beforeLoop();
  void iterate();
  void afterLoop();
  void onStop();
  std::mutex lastFrameMutex;
  uint8_t * lastFrame = nullptr;
public:
  uint8_t * getLastFrame();
};
