#include "frameCapturer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <string.h>
#include <iostream>


FrameCapturer *FrameCapturer::instance = nullptr;

FrameCapturer *FrameCapturer::Init()
{
  if (instance != nullptr)
  {
    return instance;
  }
  instance = new FrameCapturer;
  return instance;
}

void FrameCapturer::Destroy()
{
  delete instance;
  instance = nullptr;
}

FrameCapturer::FrameCapturer()
{
}

FrameCapturer::~FrameCapturer()
{
}

void FrameCapturer::initEncDecLib()
{
  avcodec_register_all();
  av_register_all();
  std::cout << "avcodec ver " << LIBAVCODEC_VERSION_MAJOR << " " << LIBAVCODEC_VERSION_MINOR << " " << LIBAVCODEC_VERSION_MICRO << "\n";
  std::cout << "avformat ver " << LIBAVFORMAT_VERSION_MAJOR << " " << LIBAVFORMAT_VERSION_MINOR << " " << LIBAVFORMAT_VERSION_MICRO << "\n";
}

void FrameCapturer::initCamera()
{
  // init camera
  device = open("/dev/video0", O_RDWR);
  if (device < 0)
  {
    std::cout << "no fd\n";
    // return 1;
  }

  // just checks
  // v4l2_capability cap;
  // if (ioctl(device, VIDIOC_QUERYCAP, &cap) < 0)
  // {
  //   std::cout << "no cap\n";
  //   // return 1;
  // }
  // if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
  //   std::cout << "can't stream\n";
  //   // return 1;
  // }

  v4l2_format format;
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  format.fmt.pix.width = width;
  format.fmt.pix.height = height;
  if (ioctl(device, VIDIOC_S_FMT, &format) < 0)
  {
    perror("format");
    // return 1;
  }
  v4l2_requestbuffers bufrequest;
  bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufrequest.memory = V4L2_MEMORY_MMAP;
  bufrequest.count = 1;
  if (ioctl(device, VIDIOC_REQBUFS, &bufrequest) < 0)
  {
    perror("buf request");
    // return 1;
  }

  v4l2_buffer queryBuffer = {0};
  queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  queryBuffer.memory = V4L2_MEMORY_MMAP;
  queryBuffer.index = 0;
  if (ioctl(device, VIDIOC_QUERYBUF, &queryBuffer) < 0)
  {
    perror("query buf");
    // return 1;
  }
  std::cout << "frame size " << queryBuffer.length << "\n";
  buffer_start = mmap(nullptr, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, device, queryBuffer.m.offset);
  if (buffer_start == MAP_FAILED)
  {
    perror("map failed");
    // return 1;
  }
  memset(buffer_start, 0, queryBuffer.length);

  bufferInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufferInfo.memory = V4L2_MEMORY_MMAP;
  bufferInfo.index = 0;
  if (ioctl(device, VIDIOC_STREAMON, &type) < 0)
  {
    perror("stream on");
    // return 1;
  }
}

void FrameCapturer::destroyCamera()
{
  // maybe unmap buffer_start or smt like this
  if (ioctl(device, VIDIOC_STREAMOFF, &type) < 0)
  {
    perror("stream off");
    // return 1;
  }
  close(device);
}

Frame FrameCapturer::getRawFrame(bool noCopy)
{
  if (ioctl(device, VIDIOC_QBUF, &bufferInfo) < 0)
  {
    perror("q buf");
    // return 1;
  }
  if (ioctl(device, VIDIOC_DQBUF, &bufferInfo) < 0)
  {
    perror("dq buf");
    // return 1
  }
  if (noCopy)
  {
    return { (uint8_t *)buffer_start, bufferInfo.length };
  }
  uint8_t *data = new uint8_t[bufferInfo.length];
  memcpy(data, buffer_start, bufferInfo.length);
  return { data, bufferInfo.length };
}

void FrameCapturer::initEncoding(std::function<void(uint8_t *buf, int buf_size)> callback)
{
  bufferWriteCallback = callback;
  // encoding preps
  swsEncCont = sws_getContext(width, height, AV_PIX_FMT_YUYV422, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
  AVCodec *codec = avcodec_find_encoder(codecId);
  if (codec == nullptr)
  {
    std::cout << "codec not found\n";
  }
  enC = avcodec_alloc_context3(codec);
  enC->width = width;
  enC->height = height;
  enC->pix_fmt = AV_PIX_FMT_YUV420P;
  enC->bit_rate = bitRate;
  enC->time_base = (AVRational){1, 30};
  enC->gop_size = 0;
  if (avcodec_open2(enC, codec, nullptr) < 0)
  {
    std::cout << "cannot open codec\n";
    return;
  }
  // muxing preps
  muxer = avformat_alloc_context();
  muxer->oformat = av_guess_format("matroska", "file.mkv", nullptr);
  if (muxer->oformat == nullptr)
  {
    std::cout << "cannot find format\n";
    return;
  }
  videoTrack = avformat_new_stream(muxer, nullptr);
  muxer->oformat->video_codec = codecId;
  avcodec_parameters_from_context(videoTrack->codecpar, enC);
  videoTrack->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  videoTrack->time_base = (AVRational){1, 30};
  videoTrack->avg_frame_rate = (AVRational){30, 1};
  muxerIOBuf = (uint8_t*)av_malloc(avioBufSize);
  muxerIO = avio_alloc_context(muxerIOBuf, avioBufSize, 1, nullptr, nullptr, staticBufferWriteCallback, nullptr);
  muxer->pb = muxerIO;
  muxer->flags = AVFMT_FLAG_CUSTOM_IO;
  AVDictionary *options = nullptr;
  av_dict_set(&options, "live", "1", 0);
  if (avformat_write_header(muxer, &options) < 0)
  {
    std::cout << "didnt init output\n";
  }
  av_dict_free(&options);
  pts = 0;
}

void FrameCapturer::captureEncodedFrame()
{

  Frame fr = getRawFrame(true);
  uint8_t * rawData = (uint8_t*)av_malloc(fr.size);
  memcpy(rawData, fr.data, fr.size);
  // // create initial frame
  AVFrame *initialFrame = av_frame_alloc();
  initialFrame->width = width;
  initialFrame->height = height;
  initialFrame->format = AV_PIX_FMT_YUYV422;
  // // create converted frame
  AVFrame *destFrame = av_frame_alloc();
  destFrame->width = width;
  destFrame->height = height;
  destFrame->format = AV_PIX_FMT_YUV420P;
  destFrame->pts = pts;
  pts++;
  av_image_alloc(destFrame->data, destFrame->linesize, width, height, AV_PIX_FMT_YUV420P, 1);

  av_image_fill_arrays(initialFrame->data, initialFrame->linesize, rawData, AV_PIX_FMT_YUYV422, width, height, 1);
  sws_scale(swsEncCont, initialFrame->data, initialFrame->linesize, 0, height, destFrame->data, destFrame->linesize);

  int sendFrRes = avcodec_send_frame(enC, destFrame);

  av_freep(&initialFrame->data[0]);
  av_freep(&destFrame->data[0]);
  av_frame_free(&initialFrame);
  av_frame_free(&destFrame);

  if (sendFrRes != 0)
  {
    if (sendFrRes == AVERROR(EAGAIN))
    {
      // std::cout << "send frame eagain\n";
    }
    else
    {
      std::cout << "send frame error\n";
      return;
    }
  }

  AVPacket *pak = av_packet_alloc();
  int recPkRes = avcodec_receive_packet(enC, pak);
  if (recPkRes != 0)
  {
    if (recPkRes == AVERROR(EAGAIN))
    {
      // std::cout << "receive packet eagain\n";
    }
    else
    {
      std::cout << "receive packet error\n";
      return;
    }
  }
  else
  {
    pak->stream_index = videoTrack->index;
    // some trickery for matroska container
    pak->pts = av_rescale_q(pak->pts, enC->time_base, videoTrack->time_base);
    pak->dts = av_rescale_q(pak->dts, enC->time_base, videoTrack->time_base);
    av_write_frame(muxer, pak);
  }
  av_packet_free(&pak);
}

void FrameCapturer::destroyEncoding()
{
  avformat_free_context(muxer);
  avio_context_free(&muxerIO);
  sws_freeContext(swsEncCont);
  avcodec_free_context(&enC);
  pts = 0;
  muxer = nullptr;
  muxerIOBuf = nullptr;
  swsEncCont = nullptr;
}

int FrameCapturer::staticBufferWriteCallback(void *opaque, uint8_t *buf, int buf_size)
{
  if (instance == nullptr)
    return 0;
  instance->bufferWriteCallback(buf, buf_size);
  return buf_size;
}

void FrameCapturer::initDecoding(std::function<int(uint8_t *buf, int buf_size)> callback)
{
  bufferReadCallback = callback;
  

  // decoding preps
  AVCodec *codec = avcodec_find_decoder(codecId);
  deC = avcodec_alloc_context3(codec);
  swsDecCont = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
  deC->width = width;
  deC->height = height;
  deC->pix_fmt = AV_PIX_FMT_YUV420P;
  deC->bit_rate = bitRate;
  if (avcodec_open2(deC, codec, nullptr) < 0)
  {
    std::cout << "cannot open codec\n";
    return;
  }

  // demuxing preps
  demuxer = avformat_alloc_context();
  demuxerIOBuf = (uint8_t*)av_malloc(avioBufSize);
  demuxerIO = avio_alloc_context(demuxerIOBuf, avioBufSize, 0, nullptr, staticBufferReadCallback, nullptr, nullptr);
  demuxer->pb = demuxerIO;
  avformat_open_input(&demuxer, nullptr, nullptr, nullptr);
}

Frame FrameCapturer::getDecodedFrame()
{
  AVPacket *pak = av_packet_alloc();
  if (av_read_frame(demuxer, pak) == 0)
  {
    int spRes = avcodec_send_packet(deC, pak);
    if (spRes != 0)
    {
      if (spRes == AVERROR(EAGAIN))
      {
        std::cout << "send packet eagain\n";
      }
      else
      {
        std::cout << "send packet error\n";
      }
    }
  }
  else
  {
    std::cout << "read frame error\n";
  }
  av_packet_free(&pak);

  AVFrame *dcFrame = av_frame_alloc();
  if (avcodec_receive_frame(deC, dcFrame) == 0)
  {
    AVFrame *destDcFrame = av_frame_alloc();
    destDcFrame->width = width;
    destDcFrame->height = height;
    destDcFrame->format = AV_PIX_FMT_RGB24;
    av_image_alloc(destDcFrame->data, destDcFrame->linesize, width, height, AV_PIX_FMT_RGB24, 1);
    sws_scale(swsDecCont, dcFrame->data, dcFrame->linesize, 0, height, destDcFrame->data, destDcFrame->linesize);
    uint32_t bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t *decodedData = new uint8_t[bufSize];
    av_image_copy_to_buffer(decodedData, bufSize, destDcFrame->data, destDcFrame->linesize, AV_PIX_FMT_RGB24, width, height, 1);
    av_freep(&destDcFrame->data[0]);
    av_frame_free(&dcFrame);
    av_frame_free(&destDcFrame);
    return { decodedData, bufSize };
  }
  av_frame_free(&dcFrame);
  return { nullptr, 0 };
}

void FrameCapturer::destroyDecoding()
{
  avformat_free_context(demuxer);
  avio_context_free(&demuxerIO);
  sws_freeContext(swsDecCont);
  avcodec_free_context(&deC);
  demuxer = nullptr;
  demuxerIOBuf = nullptr;
  swsDecCont = nullptr;
}

int FrameCapturer::staticBufferReadCallback(void *opaque, uint8_t *buf, int buf_size)
{
  if (instance == nullptr)
    return 0;
  return instance->bufferReadCallback(buf, buf_size);
}