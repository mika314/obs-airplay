#pragma once
#include <obs/obs.h>
#include <span>
#include <vector>

struct Plane
{
  std::vector<uint8_t> data;
  int linesize;
};

struct VFrame
{
  std::vector<Plane> planes;
  int width;
  int height;
  video_format format;
};

class H264Decoder
{
public:
  H264Decoder();
  ~H264Decoder();
  auto decode(std::span<const uint8_t> data) -> const VFrame *;

private:
  struct AVCodec *codec;
  struct AVCodecContext *ctx;
  struct AVFrame *yuvPicture;
  struct AVFrame *rgbPicture;
  struct AVPacket *pkt;
  struct SwsContext *swsContext = nullptr;
  uint8_t *buffer = nullptr;
  int lastWidth = 0;
  int lastHeight = 0;
  VFrame frame;
};
