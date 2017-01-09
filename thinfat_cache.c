/*!
 * @file thinfat_cache.c
 * @brief Cache layer implementation of RivieraWaves FAT driver
 * @date 2017/01/09
 * @author Hiroka IHARA
 */

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
  /*case THINFAT_CACHE_EVENT_READ_MULTIPLE:
    if (tf->si_hidden + tf->sc_reserved <= s_param && s_param < tf->si_root)
    {
      thinfat_sector_t si_next_copy = s_param + tf->sc_table_size;
      if (si_next_copy < tf->si_root)
      {
        return thinfat_phy_write_single(cache, tf->phy, si_next_copy, cache->data, THINFAT_CACHE_EVENT_READ_MULTIPLE_REPLACE);
      }
    }
    cache->state = THINFAT_CACHE_STATE_CLEAN;
    return thinfat_phy_read_multiple(cache->client, tf->phy, cache->si_replace, cache->sc_read, cache->event);
  case THINFAT_CACHE_EVENT_WRITE_MULTIPLE:
    if (tf->si_hidden + tf->sc_reserved <= s_param && s_param < tf->si_root)
    {
      thinfat_sector_t si_next_copy = s_param + tf->sc_table_size;
      if (si_next_copy < tf->si_root)
      {
        return thinfat_phy_write_single(cache, tf->phy, si_next_copy, cache->data, THINFAT_CACHE_EVENT_WRITE_MULTIPLE_REPLACE);
      }
    }
    cache->state = THINFAT_CACHE_STATE_CLEAN;
    return thinfat_phy_write_multiple(cache->client, tf->phy, cache->si_replace, cache->sc_write, cache->event);
  case THINFAT_CACHE_EVENT_READ_MULTIPLE:
    if (p_param == NULL)
    {
      //Defer notification of the last block until the PHY layer goes into idle state
      //There is not much reason behind this design decision,
      //it is just the matter of whether you need to wait for next operation to begin
      //if you immediately request something in the BLK layer callback.
      void *data = cache->data;
      return thinfat_core_callback(cache->client, cache->event, cache->si_replace + cache->sc_read - 1, &data);
    }
    else
    {
      if (*(void **)p_param == NULL)
      {
        //The first (and the last) sector(s) is(are) always read into the cache
        //Note that the first and the last sector might be the same
        *(void **)p_param = cache->data;
        return THINFAT_RESULT_OK;
      }
      else if (s_param == cache->si_replace + cache->sc_read - 2)
      {
        //Suppress callback resulting from the last sector
        //It is deferred until PHY layer is idle again
        //Destination of read is forced to the cache
        cache->state = THINFAT_CACHE_STATE_CLEAN;
        cache->si_cached = s_param;
        *(void **)p_param = cache->data;
        return THINFAT_RESULT_OK;
      }
      else
      {
        cache->state = THINFAT_CACHE_STATE_CLEAN;
        cache->si_cached = s_param;
        return thinfat_core_callback(cache->client, cache->event, s_param, p_param);
      }
    }
  case THINFAT_CACHE_EVENT_WRITE_MULTIPLE:
    if (p_param == NULL)
    {
      void *data = cache->data;

    }*/
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

/*thinfat_result_t thinfat_cached_read_multiple(void *client, thinfat_cache_t *cache, thinfat_sector_t si_read, thinfat_sector_t sc_read, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)cache->parent;
  void *data = cache->data;
  switch(cache->state)
  {
  case THINFAT_CACHE_STATE_INVALID:
    return thinfat_phy_read_multiple(cache, tf->phy, si_read, sc_read, data, THINFAT_CACHE_EVENT_READ_MULTIPLE);
  case THINFAT_CACHE_STATE_CLEAN:
    if (cache->si_cached == si_read)
    {
      thinfat_result_t res = thinfat_core_callback(client, event, si_read, &data);
      si_read++;
      sc_read--;
    }
    if (sc_read > 0)
    {
      return thinfat_phy_read_multiple(cache, tf->phy, si_read, sc_read, data, THINFAT_CACHE_EVENT_READ_MULTIPLE);
    }
    break;
  case THINFAT_CACHE_STATE_DIRTY:
    cache->event = event;
    cache->client = client;
    cache->si_replace = si_read;
    return thinfat_phy_write_single(cache, tf->phy, cache->si_cached, cache->data, THINFAT_CACHE_EVENT_READ_MULTIPLE_REPLACE);
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_cached_write_multiple(void *client, thinfat_cache_t *cache, thinfat_sector_t si_write, thinfat_sector_t sc_write, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)cache->parent;
  void *data = cache->data;
  switch(cache->state)
  {
  case THINFAT_CACHE_STATE_INVALID:
  case THINFAT_CACHE_STATE_CLEAN:
    return thinfat_phy_write_multiple(client, tf->phy, si_write, sc_write, event);
  case THINFAT_CACHE_STATE_DIRTY:
    if (cache->si_cached == si_write)
    {
      return thinfat_phy_write_multiple(cache, tf->phy, si_write, sc_write, data, THINFAT_CACHE_EVENT_WRITE_MULTIPLE);
    }
    else
    {
      cache->event = event;
      cache->client = client;
      cache->si_replace = si_write;
      return thinfat_phy_write_single(cache, tf->phy, cache->si_cached, cache->data, THINFAT_CACHE_EVENT_WRITE_MULTIPLE_REPLACE);
    }
    break;
  }
  return THINFAT_RESULT_OK;
}*/

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
      void *data = cache->data;
      return thinfat_core_callback(client, event, si_read, &data);
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
      void *data = cache->data;
      return thinfat_core_callback(client, event, si_read, &data);
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
