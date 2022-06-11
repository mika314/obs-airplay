extern "C" {
#include <obs/obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("airplay", "en-US")

static auto sourceName(void *) -> const char *
{
  return "airplay";
}

static auto sourceCreate(obs_data *, obs_source *) -> void *
{
  return nullptr;
}

static auto sourceDestroy(void *) -> void {}

static auto sourceUpdate(void *, obs_data_t *) -> void {}

static auto sourceRender(void *, gs_effect_t *) -> void {}

static auto sourceWidth(void *) -> uint32_t
{
  return 0;
}

static auto sourceHeight(void *) -> uint32_t
{
  return 0;
}

static struct obs_source_info source = {.id = "airplay",
                                        .type = OBS_SOURCE_TYPE_INPUT,
                                        .output_flags = OBS_SOURCE_VIDEO,
                                        .get_name = sourceName,
                                        .create = sourceCreate,
                                        .destroy = sourceDestroy,
                                        .get_width = sourceWidth,
                                        .get_height = sourceHeight,
                                        .update = sourceUpdate,
                                        .video_render = sourceRender};

bool obs_module_load(void)
{
  obs_register_source(&source);
  return true;
}
}
