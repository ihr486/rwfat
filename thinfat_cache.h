#ifndef THINFAT_CACHE_H
#define THINFAT_CACHE_H

#include "thinfat_common.h"

typedef enum
{
  THINFAT_CACHE_STATE_INVALID = 0,
  THINFAT_CACHE_STATE_CLEAN,
  THINFAT_CACHE_STATE_DIRTY
}
thinfat_cache_state_t;

struct thinfat_tag;

typedef struct thinfat_cache_tag
{
  struct thinfat_tag *parent;
  void *client;
  thinfat_cache_state_t state;
  thinfat_core_event_t event;
  thinfat_sector_t si_cached, si_replace;
  uint8_t data[THINFAT_SECTOR_SIZE];
}
thinfat_cache_t;

thinfat_result_t thinfat_cache_callback(thinfat_cache_t *cache, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);
thinfat_result_t thinfat_cache_init(thinfat_cache_t *cache, struct thinfat_tag *parent);
thinfat_result_t thinfat_cached_read_single(void *client, thinfat_cache_t *cache, thinfat_sector_t si_read, thinfat_core_event_t event);

#endif
