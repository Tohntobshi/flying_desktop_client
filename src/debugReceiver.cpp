#include "debugReceiver.h"
#include <iostream>


void DebugReceiver::iterate()
{
  if (!conn.connectIP("192.168.1.5","8082"))
  {
    std::cout << "debug receiver: not connected, retry in 3 seconds...\n";
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
      delete [] packet.data;
    }
  }
}

void DebugReceiver::onStop()
{
  conn.abortListenAndReceive();
}