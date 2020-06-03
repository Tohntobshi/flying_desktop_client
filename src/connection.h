#pragma once
#include <functional>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <list>

struct Header
{
  uint8_t name[3];
  uint32_t size;
};

struct Packet
{
  uint8_t * data;
  uint32_t size;
};

class Connection
{
private:
  static int registerService(uint8_t rfcommChannel, char * serviceId);
  int connectionSocket = -1;
  int abortPipe[2];
  bool isReadyToRead(int fd, int abortSignalFd);
  bool receive(int fd, int abortSignalFd, uint8_t *packet, uint32_t size);
  bool sendP(int fd, uint8_t *packet, uint32_t size);
  bool connected = false;
  std::mutex connectionMutex;
public:
  bool listenIP(char *port);
  bool listenBT(uint8_t rfcommChannel);
  bool connectIP(char *address, char *port);
  bool connectBT(char *address, uint8_t rfcommChannel);
  bool sendPacket(uint8_t *data, uint32_t size); // and delete data after call yourself
  Packet receivePacket(); // and delete data yourself
  bool isConnected(); // may be called from another thread
  void abortListenAndReceive(); // may be called from another thread
  Connection();
  ~Connection();
};
