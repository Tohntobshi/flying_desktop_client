#pragma once
#include <stdint.h>
#include "separateLoop.h"
#include "connection.h"
#include "rapidjson/document.h"

class DebugReceiver: public SeparateLoop {
private:
  Connection conn;
  // std::mutex mtx;
  void iterate();
  void onStop();
public:
  void getData();
};
