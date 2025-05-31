#include "airplay.hpp"
#include <log/log.hpp>
#include <map>
#include <string>

extern "C" {
#include <obs/obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("AirPlay", "en-US")

// Embedded locale data
static std::map<std::string, std::map<std::string, std::string>> locale_strings = {
  {"en-US", {
    {"ServerName", "Server Name"},
    {"ApplyServerName", "Apply Server Name"},
    {"ServerNameInfo", "Click 'Apply Server Name' to restart the server with the new name, or restart OBS to apply changes automatically."},
    {"MacAddressLabel", "MAC Address Settings"},
    {"MacAddressLabelDescription", "Configure which MAC Address is being used."},
    {"UseRandomMac", "Use Random MAC Address"},
    {"RandomMacInfo", "When unchecked, uses the system's MAC address. Random MAC is recommended to prevent iOS connection issues caused by device caching."}
  }},
  {"de-DE", {
    {"ServerName", "Server Name"},
    {"ApplyServerName", "Server Name anwenden"},
    {"ServerNameInfo", "Klicken Sie auf 'Server Name anwenden', um den Server mit dem neuen Namen neu zu starten, oder starten Sie OBS neu, um Änderungen automatisch anzuwenden."},
    {"MacAddressLabel", "MAC-Adresse Einstellungen"},
    {"MacAddressLabelDescription", "Konfigurieren Sie, welche MAC -Adresse verwendet wird."},
    {"UseRandomMac", "Zufällige MAC-Adresse verwenden"},
    {"RandomMacInfo", "Wenn deaktiviert, wird die System-MAC-Adresse verwendet. Zufällige MAC wird empfohlen, um iOS-Verbindungsprobleme durch Gerätecaching zu vermeiden."}
  }}
};

// Helper function to get localized text
static const char* get_text(const char* key) {
  const char* locale = obs_get_locale();
  std::string locale_str(locale ? locale : "en-US");
  
  // Try exact locale match first
  auto locale_it = locale_strings.find(locale_str);
  if (locale_it == locale_strings.end()) {
    // Try language code only (e.g., "de" from "de-DE")
    std::string lang = locale_str.substr(0, 2);
    for (const auto& [loc, strings] : locale_strings) {
      if (loc.substr(0, 2) == lang) {
        locale_it = locale_strings.find(loc);
        break;
      }
    }
  }
  
  // Fallback to English
  if (locale_it == locale_strings.end()) {
    locale_it = locale_strings.find("en-US");
  }
  
  if (locale_it != locale_strings.end()) {
    auto text_it = locale_it->second.find(key);
    if (text_it != locale_it->second.end()) {
      return text_it->second.c_str();
    }
  }
  
  return key; // Fallback to key if not found
}

static auto sourceName(void *v) -> const char *
{
  return static_cast<AirPlay *>(v)->name();
}

static auto sourceCreate(obs_data *data, obs_source *obsSource) -> void *
{
  return new AirPlay(data, obsSource);
}

static auto sourceDestroy(void *v) -> void
{
  delete static_cast<AirPlay *>(v);
}

static auto sourceUpdate(void *v, obs_data_t *data) -> void
{
  static_cast<AirPlay *>(v)->update(data);
}

static auto sourceWidth(void *v) -> uint32_t
{
  return static_cast<AirPlay *>(v)->getWidth();
}

static auto sourceHeight(void *v) -> uint32_t
{
  return static_cast<AirPlay *>(v)->getHeight();
}

static auto sourceGetDefaults(obs_data_t *data) -> void
{
  obs_data_set_default_string(data, "server_name", "OBS");
  obs_data_set_default_bool(data, "use_random_mac", true);
  obs_data_set_default_string(data, "mac_address_label", get_text("MacAddressLabelDescription"));
  obs_data_set_default_string(data, "server_name_info", get_text("ServerNameInfo"));
  obs_data_set_default_string(data, "random_mac_info", get_text("RandomMacInfo"));
}

static bool apply_settings_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
  UNUSED_PARAMETER(props);
  UNUSED_PARAMETER(property);
  
  AirPlay *airplay = static_cast<AirPlay *>(data);
  if (airplay)
  {
    airplay->apply_settings();
  }
  return false; // Don't refresh properties
}

static auto sourceGetProperties(void *data) -> obs_properties_t *
{
  obs_properties_t *props = obs_properties_create();
  
  // Server Name section
  obs_properties_add_text(props, "server_name", get_text("ServerName"), OBS_TEXT_DEFAULT);
  obs_properties_add_button(props, "apply_name", get_text("ApplyServerName"), apply_settings_clicked);
  obs_properties_add_text(props, "server_name_info", "", OBS_TEXT_INFO);
  
  // Random MAC Address section
  obs_properties_add_text(props, "mac_address_label", get_text("MacAddressLabel"), OBS_TEXT_INFO);
  obs_properties_add_bool(props, "use_random_mac", get_text("UseRandomMac"));
  obs_properties_add_text(props, "random_mac_info", "", OBS_TEXT_INFO);
  
  return props;
}

static struct obs_source_info source = {.id = "AirPlay",
                                        .type = OBS_SOURCE_TYPE_INPUT,
                                        .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO,
                                        .get_name = sourceName,
                                        .create = sourceCreate,
                                        .destroy = sourceDestroy,
                                        .get_width = sourceWidth,
                                        .get_height = sourceHeight,
                                        .update = sourceUpdate,
                                        .get_defaults = sourceGetDefaults,
                                        .get_properties = sourceGetProperties,
                                        .icon_type = OBS_ICON_TYPE_DESKTOP_CAPTURE};

bool obs_module_load(void)
{
  obs_register_source(&source);
  return true;
}
}