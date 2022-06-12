#pragma once
#include "h264-decoder.hpp"
#include <memory>
#include <thread>
#include <vector>

class AirPlay
{
public:
  AirPlay(struct obs_data *data, struct obs_source *obsSource);
  ~AirPlay();
  auto getWidth() const -> int;
  auto getHeight() const -> int;
  auto name() const -> const char *;

private:
  auto run() -> void;
  auto render(const struct h264_decode_s *data) -> void;
  auto start_raop_server(std::vector<char> hw_addr,
                         std::string name,
                         unsigned short tcp[3],
                         unsigned short udp[3],
                         bool debug_log) -> int;

  auto stop_raop_server() -> int;
  // Server callbacks
  static auto audio_flush(void *cls) -> void;
  static auto audio_get_format(void *cls,
                               unsigned char *ct,
                               unsigned short *spf,
                               bool *usingScreen,
                               bool *isMedia,
                               uint64_t *audioFormat) -> void;
  static auto audio_process(void *cls, struct raop_ntp_s *ntp, struct audio_decode_s *data) -> void;
  static auto audio_set_metadata(void *cls, const void *buffer, int buflen) -> void;
  static auto audio_set_volume(void *cls, float volume) -> void;
  static auto conn_destroy(void *cls) -> void;
  static auto conn_init(void *cls) -> void;
  static auto conn_reset(void *cls, int timeouts, bool reset_video) -> void;
  static auto conn_teardown(void *cls, bool *teardown_96, bool *teardown_110) -> void;
  static auto log_callback(void *cls, int level, const char *msg) -> void;
  static auto video_flush(void *cls) -> void;
  static auto video_process(void *cls, struct raop_ntp_s *ntp, struct h264_decode_s *data) -> void;
  static auto video_report_size(void *cls,
                                float *width_source,
                                float *height_source,
                                float *width,
                                float *height) -> void;

  struct obs_data *data;
  struct obs_source *obsSource;
  std::unique_ptr<struct obs_source_frame> obsFrame;
  Frame frame;
  H264Decoder decoder;
  bool done = false;
  std::thread thread;
  bool connections_stopped = false;
  unsigned int counter = 0;
  unsigned char compression_type = 0;
  struct raop_t *raop = NULL;
  struct dnssd_s *dnssd = NULL;
  int open_connections = 0;
  int width = 100;
  int height = 100;
};
