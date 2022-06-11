#pragma once
#include <memory>
#include <thread>
#include <vector>

class AirPlay
{
public:
  AirPlay(struct obs_data *data, struct obs_source *obsSource);
  ~AirPlay();
  auto width() const -> int;
  auto height() const -> int;
  auto name() const -> const char *;

private:
  auto run() -> void;
  auto render() -> void;

  struct obs_data *data;
  struct obs_source *obsSource;
  std::unique_ptr<struct obs_source_frame> frame;
  std::vector<unsigned char> rgba;
  bool done = false;
  std::thread thread;
};
