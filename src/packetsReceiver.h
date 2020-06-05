#pragma once
#include <stdint.h>
#include "packetsTransmitter.h"
#include "condition_variable"

class PacketsReceiver: public PacketsTransmitter {
private:
  void iterate();
  void onStop();
  std::mutex cdMutex;
  std::condition_variable cd;
public:
  using PacketsTransmitter::PacketsTransmitter;
  Packet getPacket(bool blockIfNoData);
};
