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
    return { true, 0.f, 0.f };
  }
  Document d;
  d.Parse((char *)pk.data);
  float pitchErr = d["pitchErr"].GetFloat();
  float rollErr = d["rollErr"].GetFloat();
  delete [] pk.data;
  return { false, pitchErr, rollErr };
}
