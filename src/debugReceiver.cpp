#include "debugReceiver.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

DebugInfo DebugReceiver::getInfo()
{
  Packet pk = getPacket(false);
  if (pk.size == 0)
  {
    return { true, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
  }
  Document d;
  d.Parse((char *)pk.data);
  float pitchErr = d["pe"].GetFloat();
  float rollErr = d["re"].GetFloat();
  float pitchErrChR = d["peChR"].GetFloat();
  float rollErrChR = d["reChR"].GetFloat();
  float yawSpErr = d["ySE"].GetFloat();
  float yawSpErrChR = d["ySEChR"].GetFloat();
  delete [] pk.data;
  return { false, pitchErr, rollErr, pitchErrChR, rollErrChR, yawSpErr, yawSpErrChR };
}
