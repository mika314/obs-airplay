#pragma once
#include <array>
#include <cstdint>
#include <obs/obs.h>
#include <span>
#include <vector>

struct AFrame
{
  std::vector<int16_t> data;
  speaker_layout speakers;
  int sampleRate;
};

enum class AudioCodec { aacEld, aacLc, alac, unsupported };

class AudioDecoder
{
public:
  AudioDecoder();
  ~AudioDecoder();
  auto decode(std::span<const uint8_t> data) -> const AFrame *;

private:
  struct AAC_DECODER_INSTANCE *decoder = nullptr;
  AFrame obsFrame;
  std::array<int16_t, 8096> frame;
};
