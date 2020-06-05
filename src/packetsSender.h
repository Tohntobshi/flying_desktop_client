#pragma once
#include <stdint.h>
#include "separateLoop.h"
#include "connection.h"
#include <list>
#include "condition_variable"

enum TypeOfConn {
  listenIP,
  listenBT,
  connectIP,
  connectBT
};

enum SendBehavior {
  ADD_TO_QUEUE,
  BLOCK_IF_NO_CONN,
  IGNORE_IF_NO_CONN
};

class PacketsSender: public SeparateLoop {
private:
  Connection conn;
  void iterate();
  void onStop();
  std::mutex packetsMutex;
  std::list<Packet> packetsList;
  std::mutex connCdMutex;
  std::condition_variable connCd;
  std::mutex pacCdMutex;
  std::condition_variable pacCd;
  std::string addr;
  std::string port;
  uint8_t channel;
  TypeOfConn type;
public:
  PacketsSender(std::string ipAddr, std::string port);
  PacketsSender(std::string btAddr, uint8_t channel);
  PacketsSender(std::string port);
  PacketsSender(uint8_t channel);
  void sendPacket(Packet pk, SendBehavior beh);
};
