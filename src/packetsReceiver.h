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

class PacketsReceiver: public SeparateLoop {
private:
  Connection conn;
  void iterate();
  void onStop();
  std::mutex packetsMutex;
  std::list<Packet> packetsList;
  std::mutex cdMutex;
  std::condition_variable cd;
  std::string addr;
  std::string port;
  uint8_t channel;
  TypeOfConn type;
public:
  PacketsReceiver(std::string ipAddr, std::string port);
  PacketsReceiver(std::string btAddr, uint8_t channel);
  PacketsReceiver(std::string port);
  PacketsReceiver(uint8_t channel);
  Packet getPacket(bool blockIfNoData);
};
