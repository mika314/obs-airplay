#include "h264-decoder.hpp"
#include <log/log.hpp>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

H264Decoder::H264Decoder()
  : codec(avcodec_find_decoder(AV_CODEC_ID_H264)),
    ctx(avcodec_alloc_context3(codec)),
    yuvPicture(av_frame_alloc()),
    rgbPicture(av_frame_alloc()),
    pkt(av_packet_alloc())
{
  if (!codec)
  {
    throw std::runtime_error("H264Decoder: avcodec_find_decoder failed");
  }
  if (avcodec_open2(ctx, codec, NULL) < 0)
  {
    throw std::runtime_error("H264Decoder: avcodec_open2 failed");
  }
}

H264Decoder::~H264Decoder()
{
  avcodec_close(ctx);
  av_free(ctx);
  av_free(yuvPicture);
  av_free(rgbPicture);
  av_free(pkt);
  if (swsContext)
    sws_freeContext(swsContext);
}

auto H264Decoder::decode(std::span<const uint8_t> data) -> const VFrame *
{
  pkt->data = const_cast<uint8_t *>(data.data());
  pkt->size = data.size();
  int got_picture = 0;

  //  SUGGESTION
  //  Now that avcodec_decode_video2 is deprecated and replaced
  //  by 2 calls (receive frame and send packet), this could be optimized
  //  into separate routines or separate threads.
  //  Also now that it always consumes a whole buffer some code
  //  in the caller may be able to be optimized.
  int result = avcodec_receive_frame(ctx, yuvPicture);
  if (result == 0)
      got_picture = 1;
  if (result == AVERROR(EAGAIN))
      result = 0;
  if (result == 0)
      result = avcodec_send_packet(ctx, pkt);
  // The code assumes that there is always space to add a new
  // packet. This seems risky but has always worked.
  // It should actually check if (result == AVERROR(EAGAIN)) and then keep
  // the packet around and try it again after processing the frame
  // received here.

  if (result < 0)
    return nullptr;
  if (yuvPicture->linesize[0] == 0)
    return nullptr;
  char pixDesc[255];
  av_get_pix_fmt_string(pixDesc, 255, static_cast<AVPixelFormat>(yuvPicture->format));
  if (!got_picture)
    return nullptr;

  if (yuvPicture->width != lastWidth || yuvPicture->height != lastHeight)
  {
    if (swsContext)
      sws_freeContext(swsContext);
    if (buffer)
      av_free(buffer);
    swsContext = nullptr;
    buffer = nullptr;
  }

  if (!swsContext)
  {
    swsContext = sws_getContext(yuvPicture->width,
                                yuvPicture->height,
                                static_cast<AVPixelFormat>(yuvPicture->format),
                                yuvPicture->width,
                                yuvPicture->height,
                                AV_PIX_FMT_RGBA,
                                SWS_FAST_BILINEAR,
                                NULL,
                                NULL,
                                NULL);

    buffer = static_cast<uint8_t *>(av_malloc(yuvPicture->height * yuvPicture->width * 4));
    av_image_fill_arrays(rgbPicture->data,
                         rgbPicture->linesize,
                         buffer,
                         AV_PIX_FMT_RGBA,
                         yuvPicture->width,
                         yuvPicture->height,
                         1);
    rgbPicture->width = yuvPicture->width;
    rgbPicture->height = yuvPicture->height;
    lastWidth = yuvPicture->width;
    lastHeight = yuvPicture->height;
  }

  sws_scale(swsContext,
            yuvPicture->data,
            yuvPicture->linesize,
            0,
            yuvPicture->height,
            rgbPicture->data,
            rgbPicture->linesize);

  frame.planes.resize(1);
  frame.width = rgbPicture->width;
  frame.height = rgbPicture->height;
  frame.format = VIDEO_FORMAT_RGBA;
  for (auto i = 0U; i < frame.planes.size(); ++i)
  {
    frame.planes[i].data.resize(rgbPicture->linesize[i] * rgbPicture->height);
    memcpy(
      frame.planes[i].data.data(), rgbPicture->data[i], rgbPicture->linesize[i] * rgbPicture->height);
    frame.planes[i].linesize = rgbPicture->linesize[i];
  }
  return &frame;
}
