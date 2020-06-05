#pragma once
#include <stdint.h>
#include "packetsTransmitter.h"
#include "condition_variable"


enum SendBehavior {
  ADD_TO_QUEUE,
  BLOCK_IF_NO_CONN,
  IGNORE_IF_NO_CONN
};

class PacketsSender: public PacketsTransmitter {
private:
  void iterate();
  void onStop();
  std::mutex connCdMutex;
  std::condition_variable connCd;
  std::mutex pacCdMutex;
  std::condition_variable pacCd;
public:
  using PacketsTransmitter::PacketsTransmitter;
  void sendPacket(Packet pk, SendBehavior beh);
};
