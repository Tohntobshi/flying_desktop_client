#include "separateLoop.h"

void SeparateLoop::join()
{
  thread.join();
}

void SeparateLoop::stop()
{
  std::unique_lock<std::mutex> lck(stopFlagMutex);
  stopFlag = true;
  lck.unlock();
  onStop();
}

void SeparateLoop::onStop()
{
}

void SeparateLoop::beforeLoop()
{
}

void SeparateLoop::afterLoop()
{
}


void SeparateLoop::start()
{
  std::unique_lock<std::mutex> lck(stopFlagMutex);
  stopFlag = false;
  lck.unlock();
  thread = std::thread([&]() {
    beforeLoop();
    while (!shouldStop())
    {
      iterate();
    }
    afterLoop();
  });
}

bool SeparateLoop::shouldStop()
{
  std::unique_lock<std::mutex> lck(stopFlagMutex);
  return stopFlag;
}