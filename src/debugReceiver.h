#pragma once
#include <stdint.h>
#include "packetsReceiver.h"

struct DebugInfo {
  bool noMoreInfo;
  float pitchError;
  float rollError;
};

class DebugReceiver: public PacketsReceiver {
public:
  using PacketsReceiver::PacketsReceiver;
  DebugInfo getInfo();
};
