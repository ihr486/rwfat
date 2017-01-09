#include "thinfat.h"
#include "thinfat_phy.h"
#include "thinfat_cache.h"

thinfat_result_t thinfat_cache_callback(thinfat_cache_t *cache, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)cache->parent;
  switch(event)
  {
  case THINFAT_CACHE_EVENT_READ:
    cache->si_cached = s_param;
    cache->state = THINFAT_CACHE_STATE_CLEAN;
    return thinfat_core_callback(cache->client, cache->event, s_param, p_param);
  case THINFAT_CACHE_EVENT_WRITE:
    if (tf->si_hidden + tf->sc_reserved <= s_param && s_param < tf->si_root)
    {
      thinfat_sector_t si_next_copy = s_param + tf->sc_table_size;
      if (si_next_copy < tf->si_root)
      {
        return thinfat_phy_write_single(cache, tf->phy, si_next_copy, cache->data, THINFAT_CACHE_EVENT_WRITE);
      }
    }
    if (THINFAT_IS_SECTOR_VALID(cache->si_replace))
    {
      if (cache->si_replace == cache->si_cached)
      {
        cache->state = THINFAT_CACHE_STATE_CLEAN;
        return thinfat_core_callback(cache->client, cache->event, s_param, p_param);
      }
      return thinfat_phy_read_single(cache, tf->phy, cache->si_replace, cache->data, THINFAT_CACHE_EVENT_READ);
    }
    cache->si_cached = THINFAT_INVALID_SECTOR;
    cache->state = THINFAT_CACHE_STATE_INVALID;
    return thinfat_core_callback(cache->client, cache->event, s_param, p_param);
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_cache_init(thinfat_cache_t *cache, thinfat_t *parent)
{
  cache->client = NULL;
  cache->parent = parent;
  cache->state = THINFAT_CACHE_STATE_INVALID;
  cache->si_cached = THINFAT_INVALID_SECTOR;
  cache->si_replace = THINFAT_INVALID_SECTOR;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_cached_read_single(void *client, thinfat_cache_t *cache, thinfat_sector_t si_read, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)cache->parent;
  switch(cache->state)
  {
  case THINFAT_CACHE_STATE_INVALID:
    cache->event = event;
    cache->client = client;
    return thinfat_phy_read_single(cache, tf->phy, si_read, cache->data, THINFAT_CACHE_EVENT_READ);
  case THINFAT_CACHE_STATE_CLEAN:
    if (cache->si_cached == si_read)
    {
      return thinfat_core_callback(client, event, si_read, cache->data);
    }
    else
    {
      cache->event = event;
      cache->client = client;
      return thinfat_phy_read_single(cache, tf->phy, si_read, cache->data, THINFAT_CACHE_EVENT_READ);
    }
    break;
  case THINFAT_CACHE_STATE_DIRTY:
    if (cache->si_cached == si_read)
    {
      return thinfat_core_callback(client, event, si_read, cache->data);
    }
    else
    {
      cache->event = event;
      cache->si_replace = si_read;
      cache->client = client;
      return thinfat_phy_write_single(cache, tf->phy, cache->si_cached, cache->data, THINFAT_CACHE_EVENT_WRITE);
    }
    break;
  }
  return THINFAT_RESULT_OK;
}
