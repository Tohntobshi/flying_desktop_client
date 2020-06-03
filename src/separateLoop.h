#pragma once
#include <mutex>
#include <thread>

class SeparateLoop {
private:
  std::mutex stopFlagMutex;
  bool stopFlag = false;
  std::thread thread;
protected:
  virtual void beforeLoop(); // will be
  virtual void iterate() = 0; //        called from the same
  virtual void afterLoop(); //                                separate thread

  virtual void onStop(); // will be called from the thread which called stop, may be used for unlocking some operations
  bool shouldStop();
public:
  void start();
  void stop();
  void join();
};