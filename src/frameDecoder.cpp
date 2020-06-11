#include "frameDecoder.h"
#include <iostream>

void FrameDecoder::beforeLoop()
{
  packetsReceiver = new PacketsReceiver("192.168.1.3", "8080");
  packetsReceiver->start();
  frameCapt = FrameCapturer::Init();
  frameCapt->initEncDecLib();
  frameCapt->initDecoding([&](uint8_t *buf, int size) -> int {
    Packet packet = packetsReceiver->getPacket(true);
    memcpy(buf, packet.data, packet.size);
    delete[] packet.data;
    return packet.size;
  });
}

void FrameDecoder::iterate()
{
  Frame fr = frameCapt->getDecodedFrame();
  if (fr.size != 0)
  {
    std::unique_lock<std::mutex> lck(lastFrameMutex);
    delete [] lastFrame;
    lastFrame = fr.data;
  }
}

void FrameDecoder::afterLoop()
{
  frameCapt->destroyDecoding();
  frameCapt->Destroy();
  packetsReceiver->join();
  delete packetsReceiver;
}

void FrameDecoder::onStop()
{
  packetsReceiver->stop();
}

uint8_t * FrameDecoder::getLastFrame()
{
  std::unique_lock<std::mutex> lck(lastFrameMutex);
  uint8_t * fr = lastFrame;
  lastFrame = nullptr;
  return fr;
}
