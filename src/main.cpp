#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <string.h>

// #include <thread>
// #include <chrono>
// #include <mutex>
#include "connection.h"

#include "frameCapturer.h"
// #include <list>
#include "controlsSender.h"
#include "window.h"
#include "signal.h"

int main()
{
  signal(SIGPIPE, SIG_IGN);
  // std::mutex packetsMutex;
  // std::list<Packet> packetsList;
  // std::mutex lastFrameMutex;
  // uint8_t *lastFrame = nullptr;


  ControlsSender controlSender;
  controlSender.start();

  Window * window = Window::Init(&controlSender);
  window->start();

  // std::thread receivingFramesThread([&]() {
  //   Connection conn;
  //   while (true)
  //   {
  //     if (conn.connectIP("192.168.1.5","8080") != 0)
  //     // if (conn.connectBT("B8:27:EB:CE:E6:77", 9) != 0)
  //     {
  //       std::cout << "receive frames not connected, retry in 3 seconds...\n";
  //       std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  //       continue;
  //     }
  //     while (true)
  //     {
  //       Packet pk = conn.receivePacket();
  //       if (pk.size == 0) break;
  //       std::unique_lock<std::mutex> lck(packetsMutex);
  //       packetsList.push_back(pk);
  //     }

  //   }
  // });

  // std::thread decodingThread([&]() {
  //   FrameCapturer *frameCapt = FrameCapturer::Init();
  //   frameCapt->initEncDecLib();
  //   frameCapt->initDecoding([&](uint8_t *buf, int size) -> int {
  //     while (true)
  //     {
  //       std::unique_lock<std::mutex> lck(packetsMutex);
  //       if (packetsList.size() == 0)
  //       {
  //         lck.unlock();
  //         std::this_thread::sleep_for(std::chrono::milliseconds(50));
  //       }
  //       else
  //       {
  //         Packet fr = packetsList.front();
  //         packetsList.pop_front();
  //         lck.unlock();
  //         memcpy(buf, fr.data, fr.size);
  //         delete[] fr.data;
  //         return fr.size;
  //       }
  //     }
  //   });
  //   while(true) {
  //     Frame fr = frameCapt->getDecodedFrame();
  //     if (fr.size != 0)
  //     {
  //       std::unique_lock<std::mutex> lck2(lastFrameMutex);
  //       delete [] lastFrame;
  //       lastFrame = fr.data;
  //     }
  //   }
  // });



  controlSender.join();
  window->join();
  window->Destroy();

  // decodingThread.join();
  // receivingFramesThread.join();
  return 0;
}