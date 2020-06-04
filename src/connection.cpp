#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include "connection.h"
#include "poll.h"

bdaddr_t BDADDR_ANY_VAL = {{0, 0, 0, 0, 0, 0}};
bdaddr_t BDADDR_ALL_VAL = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
bdaddr_t BDADDR_LOCAL_VAL = {{0, 0, 0, 0xff, 0xff, 0xff}};

bool Connection::listenIP(const char *port)
{
  int listeningSocket = -1;
  int isBound = -1;
  int isListening = -1;
  int yes = 1;
  addrinfo hints = { 0 };
  addrinfo *results;
  addrinfo *r;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(nullptr, port, &hints, &results) != 0) {
    std::cout << "getaddrinfo error\n";
    return false;
  }
  for (r = results; r != nullptr; r = r->ai_next)
  {
    listeningSocket = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (listeningSocket == -1) continue;
    setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    isBound = bind(listeningSocket, r->ai_addr, r->ai_addrlen);
    if (isBound == -1) continue;
    isListening = listen(listeningSocket, 1);
    if (isListening == -1) continue;
    break;
  }
  freeaddrinfo(results);
  if (listeningSocket == -1)
  {
    std::cout << "no socket\n";
    return false;
  }
  if (isBound == -1)
  {
    std::cout << "not bound\n";
    return false;
  }
  if (isListening == -1)
  {
    std::cout << "not listening\n";
    return false;
  }
  std::cout << "listening for one connection...\n";
  sockaddr_storage their_addr;
  socklen_t length = sizeof(their_addr);
  // fcntl(listeningSocket, F_SETFL, O_NONBLOCK);
  if (!isReadyToRead(listeningSocket, abortPipe[0]))
  {
    close(listeningSocket);
    return false;
  }
  connectionSocket = accept(listeningSocket, (sockaddr*)&their_addr, &length);
  std::cout << "accepted connection\n";
  close(listeningSocket);
  std::unique_lock<std::mutex> lck(connectionMutex);
  connected = true;
  return true;
}

bool Connection::listenBT(uint8_t rfcommChannel)
{
  sockaddr_rc loc_addr = {0};
  loc_addr.rc_channel = rfcommChannel;
  loc_addr.rc_family = AF_BLUETOOTH;
  loc_addr.rc_bdaddr = BDADDR_ANY_VAL;

  int listeningSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (listeningSocket == -1)
  {
    std::cout << "no listening socket\n";
    return false;
  }

  if (bind(listeningSocket, (sockaddr *)&loc_addr, sizeof(loc_addr)) != 0)
  {
    std::cout << "not bound\n";
    return false;
  }

  if (listen(listeningSocket, 1) != 0)
  {
    std::cout << "not listening\n";
    return false;
  }
  std::cout << "waiting for one connection...\n";

  sockaddr_rc rem_addr = {0};
  socklen_t rem_addr_size = sizeof(rem_addr);
  if (!isReadyToRead(listeningSocket, abortPipe[0]))
  {
    close(listeningSocket);
    return false;
  }
  connectionSocket = accept(listeningSocket, (sockaddr *)&rem_addr, &rem_addr_size);
  char string_rem_addr[1024] = {0};
  ba2str(&rem_addr.rc_bdaddr, string_rem_addr);
  std::cout << "accepted connection from " << string_rem_addr << "\n";
  close(listeningSocket);
  std::unique_lock<std::mutex> lck(connectionMutex);
  connected = true;
  return true;
}

bool Connection::connectIP(const char *address, const char *port)
{
  int isConnected = -1;
  addrinfo hints = { 0 };
  addrinfo *results;
  addrinfo *r;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(address, port, &hints, &results) != 0) {
    std::cout << "getaddrinfo error\n";
    return false;
  }
  for (r = results; r != nullptr; r = r->ai_next)
  {
    connectionSocket = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (connectionSocket == -1) continue;
    isConnected = connect(connectionSocket, r->ai_addr, r->ai_addrlen);
    if (isConnected == -1) continue;
    break;
  }
  freeaddrinfo(results);
  if (connectionSocket == -1)
  {
    std::cout << "no socket\n";
    return false;
  }
  if (isConnected == -1)
  {
    std::cout << "no connection\n";
    return false;
  }
  std::cout << "connected\n";
  std::unique_lock<std::mutex> lck(connectionMutex);
  connected = true;
  return true;
}

bool Connection::connectBT(const char *address, uint8_t rfcommChannel)
{
  sockaddr_rc rem_addr = {0};
  rem_addr.rc_channel = rfcommChannel;
  rem_addr.rc_family = AF_BLUETOOTH;
  str2ba(address, &rem_addr.rc_bdaddr);

  connectionSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (connectionSocket == -1)
  {
    std::cout << "no socket\n";
    return false;
  }
  if (connect(connectionSocket, (sockaddr *)&rem_addr, sizeof(rem_addr)) != 0)
  {
    std::cout << "not connected\n";
    return false;
  }
  std::cout << "connected\n";
  std::unique_lock<std::mutex> lck(connectionMutex);
  connected = true;
  return true;
}

bool Connection::sendPacket(uint8_t *data, uint32_t size)
{
  if (connectionSocket == -1)
  {
    return false;
  }
  Header header = {{'M', 'Y', 'H'}, size};
  uint8_t *headerAddr = (uint8_t *)&header;
  if (!sendP(connectionSocket, (uint8_t *)&header, sizeof(header)))
  {
    // perror("send error");
    close(connectionSocket);
    std::unique_lock<std::mutex> lck(connectionMutex);
    connected = false;
    return false;
  }
  if (!sendP(connectionSocket, data, size))
  {
    // perror("send error");
    close(connectionSocket);
    std::unique_lock<std::mutex> lck(connectionMutex);
    connected = false;
    return false;
  }
  return true;
}

bool Connection::sendP(int fd, uint8_t *data, uint32_t size)
{
  int bytesSent = 0;
  while (bytesSent < size)
  {
    int currentlySent = send(fd, &(data[bytesSent]), size - bytesSent, 0);
    if (currentlySent < 1)
    {
      return false;
    }
    bytesSent += currentlySent;
  }
  return true;
}

Packet Connection::receivePacket()
{
  if (connectionSocket == -1)
  {
    return { nullptr, 0 };
  }
  Header header = {0};
  if (!receive(connectionSocket, abortPipe[0], (uint8_t *)&header, sizeof(header)))
  {
    close(connectionSocket);
    std::unique_lock<std::mutex> lck(connectionMutex);
    connected = false;
    return { nullptr, 0 };
  }
  if (header.name[0] != 'M' || header.name[1] != 'Y' || header.name[2] != 'H')
  {
    close(connectionSocket);
    std::unique_lock<std::mutex> lck(connectionMutex);
    connected = false;
    return { nullptr, 0 };
  }
  uint8_t *packet = new uint8_t[header.size];
  if (!receive(connectionSocket, abortPipe[0], packet, header.size))
  {
    delete [] packet;
    close(connectionSocket);
    std::unique_lock<std::mutex> lck(connectionMutex);
    connected = false;
    return { nullptr, 0 };
  }
  return { packet, header.size };
}

bool Connection::receive(int fd, int abortSignalFd, uint8_t *packet, uint32_t size)
{
  int bytesReceived = 0;
  while (bytesReceived < size)
  {
    if (!isReadyToRead(fd, abortSignalFd))
    {
      return false;
    }
    int currentlyReceived = recv(fd, &(packet[bytesReceived]), size - bytesReceived, 0);
    if (currentlyReceived < 1)
    {
      return false;
    }
    bytesReceived += currentlyReceived;
  }
  return true;
}

bool Connection::isReadyToRead(int fd, int abortSignalFd)
{
  pollfd descriptors[2];
  descriptors[0].fd = fd;
  descriptors[0].events = POLLIN;
  descriptors[1].fd = abortSignalFd;
  descriptors[1].events = POLLIN;
  int numEvts = poll(descriptors, 2, -1);
  if (numEvts == 1 && descriptors[0].revents & POLLIN)
  {
    return true;
  }
  return false;
}

void Connection::abortListenAndReceive()
{
  char a = 0;
  write(abortPipe[1], &a, 1);
}

bool Connection::isConnected()
{
  std::unique_lock<std::mutex> lck(connectionMutex);
  return connected;
}

Connection::Connection()
{
  pipe(abortPipe);
}

Connection::~Connection()
{
  close(abortPipe[0]);
  close(abortPipe[1]);
  // sdp_close(sdpSession);
  close(connectionSocket);
}