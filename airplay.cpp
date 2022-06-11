#include "airplay.hpp"
#include <log/log.hpp>
#include <obs/obs.h>

AirPlay::AirPlay(struct obs_data *data, struct obs_source *obsSource)
  : data(data),
    obsSource(obsSource),
    frame(std::make_unique<obs_source_frame>()),
    thread(&AirPlay::run, this)
{
  rgba.resize(100 * 100 * 4);

  for (auto x = 0; x < 100; ++x)
    for (auto y = 0; y < 100; ++y)
      for (auto c = 0; c < 4; ++c)
        rgba[(x * 100 + y) * 4 + c] = 0x80;

  frame->data[0] = rgba.data();
  frame->linesize[0] = 100 * 4;
  frame->width = 100;
  frame->height = 100;
  frame->format = VIDEO_FORMAT_RGBA;
  frame->timestamp = 0;
  obs_source_output_video(obsSource, frame.get());
}

auto AirPlay::render() -> void
{
  if (!obsSource)
    return;
  LOG("rendering");
  frame->timestamp++;
  obs_source_output_video(obsSource, frame.get());
}

auto AirPlay::width() const -> int
{
  return 100;
}

auto AirPlay::height() const -> int
{
  return 100;
}

auto AirPlay::name() const -> const char *
{
  return "AirPlay";
}

auto AirPlay::run() -> void
{
  while (!done)
  {
    render();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

AirPlay::~AirPlay()
{
  done = true;
  thread.join();
}
