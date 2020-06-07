#include "controlsSender.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

void ControlsSender::setControl(uint8_t control, bool unique)
{
  if (unique) {
    std::unique_lock<std::mutex> lck(mtx);
    if (lastUniqueControl == control) return;
    lastUniqueControl = control;
    lck.unlock();
  }
  sendCommand(control);
}

void ControlsSender::handleEvent(SDL_Event & event)
{
  if (event.type == SDL_KEYDOWN)
  {
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_I:
      setControl(INCLINE_FORW, true);
      break;
    case SDL_SCANCODE_K:
      setControl(INCLINE_BACKW, true);
      break;
    case SDL_SCANCODE_J:
      setControl(INCLINE_LEFT, true);
      break;
    case SDL_SCANCODE_L:
      setControl(INCLINE_RIGHT, true);
      break;
    }
  }
  if (event.type == SDL_KEYUP)
  {
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_I:
    case SDL_SCANCODE_K:
    case SDL_SCANCODE_J:
    case SDL_SCANCODE_L:
      setControl(INCLINE_DEFAULT, true);
      break;
    }
  }
}

void ControlsSender::sendCommand(uint8_t key)
{
  Document d;
  Value& rootObj = d.SetObject();
  rootObj.AddMember("t", key, d.GetAllocator());
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  const char * json = buffer.GetString();
  uint32_t size = buffer.GetSize();
  uint8_t * buf = new uint8_t[size + 1];
  memcpy(buf, json, size);
  buf[size] = 0;
  sendPacket({ buf, size + 1 }, IGNORE_IF_NO_CONN);
}

template<typename T>
void ControlsSender::_sendValue(uint8_t key, T val)
{
  Document d;
  Value& rootObj = d.SetObject();
  rootObj.AddMember("t", key, d.GetAllocator());
  rootObj.AddMember("v", val, d.GetAllocator());
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  const char * json = buffer.GetString();
  uint32_t size = buffer.GetSize();
  uint8_t * buf = new uint8_t[size + 1];
  memcpy(buf, json, size);
  buf[size] = 0;
  sendPacket({ buf, size + 1 }, IGNORE_IF_NO_CONN);
}

void ControlsSender::sendValue(uint8_t key, int value)
{
  _sendValue(key, value);
}

void ControlsSender::sendValue(uint8_t key, float value)
{
  _sendValue(key, value);
}