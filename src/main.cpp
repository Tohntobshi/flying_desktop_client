#include <iostream>

#include "frameDecoder.h"
#include "controlsSender.h"
#include "window.h"
#include "signal.h"

int main()
{
  signal(SIGPIPE, SIG_IGN);

  FrameDecoder frameDecoder;
  frameDecoder.start();

  ControlsSender controlSender;
  controlSender.start();

  Window * window = Window::Init(&controlSender, &frameDecoder);

  window->start();
  
  window->join();
  window->Destroy();

  controlSender.join();

  frameDecoder.join();
  return 0;
}