#include <iostream>

#include "frameDecoder.h"
#include "controlsSender.h"
#include "window.h"
#include "signal.h"
#include "debugReceiver.h"

int main()
{
  signal(SIGPIPE, SIG_IGN);

  DebugReceiver debugReceiver("192.168.1.3", "8082");
  debugReceiver.start();

  // FrameDecoder frameDecoder;
  // frameDecoder.start();

  ControlsSender controlSender("192.168.1.3", "8081");
  controlSender.start();

  Window * window = Window::Init(&controlSender, nullptr, &debugReceiver);

  window->start();
  
  window->join();
  window->Destroy();

  controlSender.join();

  // frameDecoder.join();

  debugReceiver.join();
  return 0;
}