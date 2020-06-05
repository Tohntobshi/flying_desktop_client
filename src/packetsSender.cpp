#include "packetsSender.h"
#include <iostream>

PacketsSender::PacketsSender(std::string ipAddr, std::string p):
addr(ipAddr), port(p), type(connectIP)
{

}

PacketsSender::PacketsSender(std::string btAddr, uint8_t ch):
addr(btAddr), channel(ch), type(connectBT)
{

}

PacketsSender::PacketsSender(std::string p):
port(p), type(listenIP)
{

}

PacketsSender::PacketsSender(uint8_t ch):
channel(ch), type(listenBT)
{

}

void PacketsSender::iterate()
{
  bool connected = false;
  switch (type)
  {
  case connectIP:
    connected = conn.connectIP(addr.c_str(), port.c_str());
    break;
  case connectBT:
    connected = conn.connectBT(addr.c_str(), channel);
    break;
  case listenIP:
    connected = conn.listenIP(port.c_str());
    break;
  case listenBT:
    connected = conn.listenBT(channel);
    break;
  
  }
  if (!connected)
  {
    std::cout << "packets sender: not connected, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  while (!shouldStop())
  {
    std::unique_lock<std::mutex> packetsLck(packetsMutex);
    if (packetsList.size() == 0)
    {
      packetsLck.unlock();
      std::unique_lock<std::mutex> pacCdLck(pacCdMutex);
      pacCd.wait(pacCdLck);
    }
    else
    {
      Packet packet = packetsList.front();
      packetsList.pop_front();
      packetsLck.unlock();
      bool sent = conn.sendPacket(packet.data, packet.size);
      delete [] packet.data;
      if (!sent) return;
    }
  }
}

void PacketsSender::onStop()
{
  conn.abortListenAndReceive();
  connCd.notify_all();
  pacCd.notify_all();
}


void PacketsSender::sendPacket(Packet pk, SendBehavior beh)
{
  if (beh == ADD_TO_QUEUE)
  {
    std::unique_lock<std::mutex> lck(packetsMutex);
    packetsList.push_back(pk);
    pacCd.notify_all();
    return;
  }
  if (beh == IGNORE_IF_NO_CONN)
  {
    if (conn.isConnected())
    {
      std::unique_lock<std::mutex> lck(packetsMutex);
      packetsList.push_back(pk);
      pacCd.notify_all();
    }
    return;
  }
  if (beh == BLOCK_IF_NO_CONN)
  {
    if (!conn.isConnected())
    {
      std::unique_lock<std::mutex> lck(connCdMutex);
      connCd.wait(lck);
    }
    std::unique_lock<std::mutex> lck(packetsMutex);
    packetsList.push_back(pk);
    pacCd.notify_all();
    return;
  }
}
