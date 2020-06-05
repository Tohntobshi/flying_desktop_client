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

class PacketsTransmitter: public SeparateLoop {
protected:
  Connection conn;
  std::mutex packetsMutex;
  std::list<Packet> packetsList;
  std::string addr;
  std::string port;
  uint8_t channel;
  TypeOfConn type;
public:
  PacketsTransmitter(std::string ipAddr, std::string port);
  PacketsTransmitter(std::string btAddr, uint8_t channel);
  PacketsTransmitter(std::string port);
  PacketsTransmitter(uint8_t channel);
};
