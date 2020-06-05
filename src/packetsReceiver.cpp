#include "packetsReceiver.h"
#include <iostream>

void PacketsReceiver::iterate()
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
    std::cout << "packets receiver: not connected, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  while (!shouldStop())
  {
    Packet packet = conn.receivePacket();
    if (packet.size == 0)
    {
      return;
    }
    else
    {
      std::unique_lock<std::mutex> lck(packetsMutex);
      packetsList.push_back(packet);
      cd.notify_all();
    }
  }
}

void PacketsReceiver::onStop()
{
  conn.abortListenAndReceive();
  cd.notify_all();
}

Packet PacketsReceiver::getPacket(bool blockIfNoData)
{
  std::unique_lock<std::mutex> packetsLck(packetsMutex);
  if (packetsList.size() == 0)
  {
    packetsLck.unlock();
    if (shouldStop()) return { nullptr, 0 };
    if (blockIfNoData)
    {
      std::unique_lock<std::mutex> cdLck(cdMutex);
      cd.wait(cdLck);
      return getPacket(true);
    }
    else
    {
      return { nullptr, 0 };
    }
  }
  else
  {
    Packet packet = packetsList.front();
    packetsList.pop_front();
    packetsLck.unlock();
    return packet;
  }
}