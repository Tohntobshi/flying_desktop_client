#pragma once
#include <cstdint>
#include <linux/videodev2.h>
#include <functional>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

struct Frame {
  uint8_t * data;
  uint32_t size;
};

class FrameCapturer {
public:
  static FrameCapturer * Init();
  static void Destroy();
  void initEncDecLib();
  void initCamera();
  void destroyCamera();
  Frame getRawFrame(bool noCopy);
  void initEncoding(std::function<void(uint8_t *buf, int buf_size)> onBufferReceive);
  void destroyEncoding();
  void captureEncodedFrame();
  void initDecoding(std::function<int(uint8_t *buf, int buf_size)> onBufferDemand);
  void destroyDecoding();
  Frame getDecodedFrame();
private:
  AVCodecID codecId = AV_CODEC_ID_MPEG4;
  int width = 640;
  int height = 480;
  int64_t bitRate = 1500000;
  int64_t pts = 0;
  AVStream * videoTrack = nullptr;
  AVFormatContext * muxer = nullptr;
  AVFormatContext * demuxer = nullptr;
  AVIOContext * muxerIO = nullptr;
  AVIOContext * demuxerIO = nullptr;
  uint8_t * muxerIOBuf = nullptr;
  uint8_t * demuxerIOBuf = nullptr;
  int avioBufSize = 32 * 1024;
  SwsContext * swsEncCont = nullptr;
  SwsContext * swsDecCont = nullptr;
  AVCodecContext *enC = nullptr;
  AVCodecContext *deC = nullptr;
  std::function<void(uint8_t *buf, int buf_size)> bufferWriteCallback;
  std::function<int(uint8_t *buf, int buf_size)> bufferReadCallback;
  static FrameCapturer * instance;
  static int staticBufferWriteCallback(void *opaque, uint8_t *buf, int buf_size);
  static int staticBufferReadCallback(void *opaque, uint8_t *buf, int buf_size);
  int device = -1;
  v4l2_buffer bufferInfo = { 0 };
  uint32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  void * buffer_start = nullptr;
  
  FrameCapturer();
  ~FrameCapturer();
};
