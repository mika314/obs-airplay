#include "airplay.hpp"
#include <chrono>
#include <log/log.hpp>
#include <obs/obs.h>

#include <assert.h>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <stddef.h>
#include <string>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>

#include <ifaddrs.h>
#include <sys/socket.h>
#ifdef __linux__
#include <netpacket/packet.h>
#else
#include <net/if_dl.h>
#endif

#include "dnssd.h"
#include "logger.h"
#include "raop.h"
#include "stream.h"

#define DEFAULT_NAME "OBS"
#define DEFAULT_DEBUG_LOG false
#define NTP_TIMEOUT_LIMIT 5
#define LOWEST_ALLOWED_PORT 1024
#define HIGHEST_PORT 65535

static std::string server_name = DEFAULT_NAME;
static unsigned int max_ntp_timeouts = NTP_TIMEOUT_LIMIT;

static std::string find_mac()
{
  /*  finds the MAC address of a network interface *
   *  in a Linux, *BSD or macOS system.            */
  std::string mac = "";
  struct ifaddrs *ifap, *ifaptr;
  int non_null_octets = 0;
  unsigned char octet[6];
  if (getifaddrs(&ifap) == 0)
  {
    for (ifaptr = ifap; ifaptr != NULL; ifaptr = ifaptr->ifa_next)
    {
      if (ifaptr->ifa_addr == NULL)
        continue;
#ifdef __linux__
      if (ifaptr->ifa_addr->sa_family != AF_PACKET)
        continue;
      struct sockaddr_ll *s = (struct sockaddr_ll *)ifaptr->ifa_addr;
      for (int i = 0; i < 6; i++)
      {
        if ((octet[i] = s->sll_addr[i]) != 0)
          non_null_octets++;
      }
#else /* macOS and *BSD */
      if (ifaptr->ifa_addr->sa_family != AF_LINK)
        continue;
      ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)ifaptr->ifa_addr);
      for (int i = 0; i < 6; i++)
      {
        if ((octet[i] = *ptr) != 0)
          non_null_octets++;
        ptr++;
      }
#endif
      if (non_null_octets)
      {
        mac.erase();
        char str[3];
        for (int i = 0; i < 6; i++)
        {
          sprintf(str, "%02x", octet[i]);
          mac = mac + str;
          if (i < 5)
            mac = mac + ":";
        }
        break;
      }
    }
  }
  freeifaddrs(ifap);
  return mac;
}

#define MULTICAST 0
#define LOCAL 1
#define OCTETS 6
static std::string random_mac()
{
  char str[3];
  int octet = rand() % 64;
  octet = (octet << 1) + LOCAL;
  octet = (octet << 1) + MULTICAST;
  snprintf(str, 3, "%02x", octet);
  std::string mac_address(str);
  for (int i = 1; i < OCTETS; i++)
  {
    mac_address = mac_address + ":";
    octet = rand() % 256;
    snprintf(str, 3, "%02x", octet);
    mac_address = mac_address + str;
  }
  return mac_address;
}

static int parse_hw_addr(std::string str, std::vector<char> &hw_addr)
{
  for (auto i = 0U; i < str.length(); i += 3)
  {
    hw_addr.push_back((char)stol(str.substr(i), NULL, 16));
  }
  return 0;
}

auto AirPlay::stop_raop_server() -> int
{
  if (raop)
  {
    raop_destroy(raop);
    raop = NULL;
  }
  if (dnssd)
  {
    dnssd_unregister_raop(dnssd);
    dnssd_unregister_airplay(dnssd);
    dnssd_destroy(dnssd);
    dnssd = NULL;
  }
  return 0;
}

auto AirPlay::start_raop_server(std::vector<char> hw_addr,
                                std::string name,
                                unsigned short display[5],
                                unsigned short tcp[3],
                                unsigned short udp[3],
                                bool debug_log) -> int
{
  raop_callbacks_t raop_cbs;
  memset(&raop_cbs, 0, sizeof(raop_cbs));
  raop_cbs.cls = this;
  raop_cbs.conn_init = conn_init;
  raop_cbs.conn_destroy = conn_destroy;
  raop_cbs.conn_reset = conn_reset;
  raop_cbs.conn_teardown = conn_teardown;
  raop_cbs.audio_process = audio_process;
  raop_cbs.video_process = video_process;
  raop_cbs.audio_flush = audio_flush;
  raop_cbs.video_flush = video_flush;
  raop_cbs.audio_set_volume = audio_set_volume;
  raop_cbs.audio_get_format = audio_get_format;
  raop_cbs.video_report_size = video_report_size;
  raop_cbs.audio_set_metadata = audio_set_metadata;

  /* set max number of connections = 2 */
  raop = raop_init(2, &raop_cbs);
  if (raop == NULL)
  {
    LOG("Error initializing raop!");
    return -1;
  }

  /* write desired display pixel width, pixel height, refresh_rate, max_fps, overscanned.  */
  /* use 0 for default values 1920,1080,60,30,0; these are sent to the Airplay client      */

  if (display[0])
    raop_set_plist(raop, "width", (int)display[0]);
  if (display[1])
    raop_set_plist(raop, "height", (int)display[1]);
  if (display[2])
    raop_set_plist(raop, "refreshRate", (int)display[2]);
  if (display[3])
    raop_set_plist(raop, "maxFPS", (int)display[3]);
  if (display[4])
    raop_set_plist(raop, "overscanned", (int)display[4]);

  raop_set_plist(raop, "max_ntp_timeouts", max_ntp_timeouts);

  /* network port selection (ports listed as "0" will be dynamically assigned) */
  raop_set_tcp_ports(raop, tcp);
  raop_set_udp_ports(raop, udp);

  raop_set_log_callback(raop, log_callback, NULL);
  raop_set_log_level(raop, debug_log ? RAOP_LOG_DEBUG : LOGGER_INFO);

  unsigned short port = raop_get_port(raop);
  raop_start(raop, &port);
  raop_set_port(raop, port);

  int error;
  dnssd = dnssd_init(name.c_str(), strlen(name.c_str()), hw_addr.data(), hw_addr.size(), &error);
  if (error)
  {
    LOG("Could not initialize dnssd library!");
    stop_raop_server();
    return -2;
  }

  raop_set_dnssd(raop, dnssd);

  dnssd_register_raop(dnssd, port);
  if (tcp[2])
  {
    port = tcp[2];
  }
  else
  {
    port = (port != HIGHEST_PORT ? port + 1 : port - 1);
  }
  dnssd_register_airplay(dnssd, port);

  return 0;
}

// Server callbacks
auto AirPlay::conn_init(void *cls) -> void
{
  LOG(__func__);

  auto self = static_cast<AirPlay *>(cls);
  self->open_connections++;
  self->connections_stopped = false;
  LOG("Open connections:", self->open_connections);
  // video_renderer_update_background(1);
}

auto AirPlay::conn_destroy(void *cls) -> void
{
  LOG(__func__);
  auto self = static_cast<AirPlay *>(cls);
  // video_renderer_update_background(-1);
  self->open_connections--;
  LOG("Open connections:", self->open_connections);
  if (!self->open_connections)
  {
    self->connections_stopped = true;
  }
}

auto AirPlay::conn_reset(void *cls, int timeouts, bool reset_video) -> void
{
  auto self = static_cast<AirPlay *>(cls);
  LOG("***ERROR lost connection with client (network problem?)");
  if (timeouts)
  {
    LOG("   Client no-response limit of %d timeouts (%d seconds) reached:", timeouts, 3 * timeouts);
    LOG("   Sometimes the network connection may recover after a longer delay:\n"
        "   the default timeout limit n = %d can be changed with the \"-reset n\" option",
        NTP_TIMEOUT_LIMIT);
  }
  printf("reset_video %d\n", (int)reset_video);
  raop_stop(self->raop);
}

auto AirPlay::conn_teardown(void * /*cls*/, bool *teardown_96, bool *teardown_110) -> void
{
  LOG(__func__, *teardown_96, *teardown_110);
}

auto AirPlay::audio_process(void * /*cls*/, raop_ntp_t * /*ntp*/, audio_decode_struct * /*data*/) -> void
{
  LOG(__func__);
}

auto AirPlay::video_process(void * /*cls*/, raop_ntp_t * /*ntp*/, h264_decode_struct * /*data*/) -> void
{
  LOG(__func__);
}

auto AirPlay::audio_flush(void * /*cls*/) -> void 
{
  LOG(__func__);
}

auto AirPlay::video_flush(void * /*cls*/) -> void 
{
  LOG(__func__);
}

auto AirPlay::audio_set_volume(void * /*cls*/, float volume) -> void 
{
  LOG(__func__, volume);
}

auto AirPlay::audio_get_format(void * /*cls*/,
                               unsigned char *ct,
                               unsigned short *spf,
                               bool *usingScreen,
                               bool *isMedia,
                               uint64_t *audioFormat) -> void
{
  unsigned char type;
  LOG("ct=%d spf=%d usingScreen=%d isMedia=%d  audioFormat=0x%lx",
      *ct,
      *spf,
      *usingScreen,
      *isMedia,
      (unsigned long)*audioFormat);
  switch (*ct)
  {
  case 2: type = 0x20; break;
  case 8: type = 0x80; break;
  default: type = 0x10; break;
  }
  (void)type;
}

auto AirPlay::video_report_size(void * /*cls*/,
                                float *width_source,
                                float *height_source,
                                float *width,
                                float *height) -> void
{
  LOG("video_report_size:", *width_source, *height_source, *width, *height);
}

auto AirPlay::audio_set_metadata(void * /*cls*/, const void *buffer, int buflen) -> void
{
  LOG(__func__, buflen);
  unsigned char mark[] = {0x00, 0x00, 0x00}; /*daap seperator mark */
  if (buflen > 4)
  {
    LOG("==============Audio Metadata=============");
    const unsigned char *metadata = (const unsigned char *)buffer;
    const char *tag = (const char *)buffer;
    int len;
    metadata += 4;
    for (int i = 4; i < buflen; i++)
    {
      if (memcmp(metadata, mark, 3) == 0 && (len = (int)*(metadata + 3)))
      {
        bool found_text = true;
        if (strcmp(tag, "asal") == 0)
        {
          LOG("Album: ");
        }
        else if (strcmp(tag, "asar") == 0)
        {
          LOG("Artist: ");
        }
        else if (strcmp(tag, "ascp") == 0)
        {
          LOG("Composer: ");
        }
        else if (strcmp(tag, "asgn") == 0)
        {
          LOG("Genre: ");
        }
        else if (strcmp(tag, "minm") == 0)
        {
          LOG("Title: ");
        }
        else
        {
          found_text = false;
        }
        if (found_text)
        {
          const unsigned char *text = metadata + 4;
          for (int j = 0; j < len; j++)
          {
            LOG(*text);
            text++;
          }
        }
      }
      metadata++;
      tag++;
    }
  }
}

auto AirPlay::log_callback(void * /*cls*/, int level, const char *msg) -> void
{
  switch (level)
  {
  case LOGGER_DEBUG: {
    LOG("D", msg);
    break;
  }
  case LOGGER_WARNING: {
    LOG("W", msg);
    break;
  }
  case LOGGER_INFO: {
    LOG("I", msg);
    break;
  }
  case LOGGER_ERR: {
    LOG("E", msg);
    break;
  }
  default: break;
  }
}

AirPlay::AirPlay(struct obs_data *data, struct obs_source *obsSource)
  : data(data),
    obsSource(obsSource),
    frame(std::make_unique<obs_source_frame>()),
    thread(&AirPlay::run, this)
{
}

auto AirPlay::render() -> void
{
  if (!obsSource)
    return;

  rgba.resize(100 * 100 * 4);

  frame->data[0] = rgba.data();
  frame->linesize[0] = 100 * 4;
  frame->width = 100;
  frame->height = 100;
  frame->format = VIDEO_FORMAT_RGBA;

  // set current time in ns
  const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
  frame->timestamp = nowNs;

  const auto whiteLine = static_cast<int>(nowNs / 80'000'000 % 100);
  for (auto x = 0; x < 100; ++x)
    for (auto y = 0; y < 100; ++y)
    {
      if (x == whiteLine)
      {
        rgba[(x * 100 + y) * 4 + 0] = 0xff;
        rgba[(x * 100 + y) * 4 + 1] = 0xff;
        rgba[(x * 100 + y) * 4 + 2] = 0xff;
        rgba[(x * 100 + y) * 4 + 3] = 0xff;
        continue;
      }
      rgba[(x * 100 + y) * 4 + 0] = rand() % 256;
      rgba[(x * 100 + y) * 4 + 1] = rand() % 256;
      rgba[(x * 100 + y) * 4 + 2] = rand() % 256;
      rgba[(x * 100 + y) * 4 + 3] = 0x80;
    }
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
  std::vector<char> server_hw_addr;
  bool use_random_hw_addr = false;
  bool debug_log = DEFAULT_DEBUG_LOG;
  unsigned short display[5] = {0}, tcp[3] = {0}, udp[3] = {0};

#ifdef SUPPRESS_AVAHI_COMPAT_WARNING
  // suppress avahi_compat nag message.  avahi emits a "nag" warning (once)
  // if  getenv("AVAHI_COMPAT_NOWARN") returns null.
  static char avahi_compat_nowarn[] = "AVAHI_COMPAT_NOWARN=1";
  if (!getenv("AVAHI_COMPAT_NOWARN"))
    putenv(avahi_compat_nowarn);
#endif

  if (udp[0])
    LOG("using network ports UDP", udp[0], udp[1], udp[2], "TCP", tcp[0], tcp[1], tcp[2]);

  std::string mac_address;
  if (!use_random_hw_addr)
    mac_address = find_mac();
  if (mac_address.empty())
  {
    srand(time(NULL) * getpid());
    mac_address = random_mac();
    LOG("using randomly-generated MAC address", mac_address);
  }
  else
  {
    LOG("using system MAC address", mac_address);
  }
  parse_hw_addr(mac_address, server_hw_addr);
  mac_address.clear();

  connections_stopped = true;
  if (start_raop_server(server_hw_addr, server_name, display, tcp, udp, debug_log) != 0)
  {
    LOG("start_raop_server failed");
    return;
  }
  counter = 0;
  compression_type = 0;

  while (!done)
  {
    render();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 30));
  }

  LOG("Stopping...");
  stop_raop_server();
}

AirPlay::~AirPlay()
{
  done = true;
  thread.join();
}
