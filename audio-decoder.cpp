#include "audio-decoder.hpp"
#include <fdk-aac/aacdecoder_lib.h>
#include <log/log.hpp>

auto AudioDecoder::decode(std::span<const uint8_t> data) -> const AFrame *
{
  auto c = [=]() {
    switch (data[0])
    {
    case 0x8c:
    case 0x8d:
    case 0x8e:
    case 0x80:
    case 0x81:
    case 0x82: return AudioCodec::aacEld;
    case 0xff: return AudioCodec::aacLc;
    case 0x20: return AudioCodec::alac;
    }
    LOG("Unknown audio codec:", data[0]);
    return AudioCodec::unsupported;
  }();

  if (c == AudioCodec::alac || c == AudioCodec::unsupported)
  {
    LOG("audio-format is not supported");
    return nullptr;
  }

  UINT bytesValid = data.size();
  {
    uint8_t *d[2] = {const_cast<uint8_t *>(data.data()), nullptr};
    UINT size[2] = {static_cast<UINT>(data.size()), 0};
    auto err = aacDecoder_Fill(decoder, d, size, &bytesValid);
    if (err != AAC_DEC_OK)
    {
      LOG("aacDecoder_Fill failed:", err);
      return nullptr;
    }
  }
  {
    auto err = aacDecoder_DecodeFrame(decoder, frame.data(), frame.size(), 0);
    if (err != AAC_DEC_OK)
    {
      LOG("aacDecoder_DecodeFrame failed:", err);
      return nullptr;
    }
  }
  {
    auto info = aacDecoder_GetStreamInfo(decoder);
    if (info == nullptr)
    {
      LOG("aacDecoder_GetStreamInfo failed");
      return nullptr;
    }
    obsFrame.sampleRate = info->sampleRate;
    switch (info->channelConfig)
    {
    case 1: // mono
      obsFrame.speakers = SPEAKERS_MONO;
      break;
    case 2: // stereo
      obsFrame.speakers = SPEAKERS_STEREO;
      break;
    default: LOG("Unknown channel config:", info->channelConfig); return nullptr;
    }
    const auto samples = info->numChannels * info->frameSize;
    obsFrame.data.clear();
    obsFrame.data.insert(obsFrame.data.end(), frame.data(), frame.data() + samples);
  }
  return &obsFrame;
}

AudioDecoder::AudioDecoder() : decoder(aacDecoder_Open(TT_MP4_RAW, 1))
{
  {
    // AAC-ELD 44100 STEREO
    UCHAR conf[] = {0xF8, 0xE8, 0x50, 0x00};
    UCHAR *conf_array[1] = {conf};
    UINT length = 4;
    auto err = aacDecoder_ConfigRaw(decoder, conf_array, &length);
    if (err != AAC_DEC_OK)
    {
      LOG("aacDecoder_ConfigRaw failed:", err);
      return;
    }
  }
}

AudioDecoder::~AudioDecoder()
{
  aacDecoder_Close(decoder);
}
