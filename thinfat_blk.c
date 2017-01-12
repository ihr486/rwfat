/*!
 * @file thinfat_blk.c
 * @brief thinFAT BLK layer implementation
 * @date 2017/01/08
 * @author Hiroka IHARA
 */
#include "thinfat.h"
#include "thinfat_phy.h"
#include "thinfat_blk.h"
#include "thinfat_cache.h"

#include <stdlib.h>
#include <string.h>

static inline thinfat_cluster_t thinfat_stoc(thinfat_t *tf, thinfat_sector_t si)
{
  return ((si - tf->si_data) >> tf->ctos_shift) + 2;
}

static inline thinfat_sector_t thinfat_ctos(thinfat_t *tf, thinfat_cluster_t ci)
{
  return ((ci - 2) << tf->ctos_shift) + tf->si_data;
}

thinfat_result_t thinfat_blk_callback(thinfat_blk_t *blk, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)blk->parent;
  thinfat_result_t res;
  blk->state = THINFAT_BLK_STATE_IDLE;
  switch(event)
  {
  case THINFAT_BLK_EVENT_LOOKUP:
    if (tf->type == THINFAT_TYPE_FAT32)
    {
      blk->ci_current = thinfat_read_u32(*(void **)p_param, (blk->ci_current * 4) % THINFAT_SECTOR_SIZE);
      printf("Next cluster = 0x%08X\n", blk->ci_current);
    }
    else
    {
      blk->ci_current = thinfat_read_u16(*(void **)p_param, (blk->ci_current * 2) % THINFAT_SECTOR_SIZE);
      if (blk->ci_current >= 0xFFF7)
      {
        blk->ci_current = THINFAT_INVALID_CLUSTER;
      }
    }
    blk->so_current += (1 << tf->ctos_shift);
    blk->state = THINFAT_BLK_STATE_IDLE;
    if (THINFAT_IS_CLUSTER_VALID(blk->ci_current))
      return thinfat_blk_seek(blk->seek_client, blk, blk->so_seek, blk->seek_event);
    else
      return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
  case THINFAT_BLK_EVENT_READ_SINGLE:
    if ((res = thinfat_core_callback(blk->client, blk->event, s_param, *(void **)p_param)) == THINFAT_RESULT_OK)
    {
      if (--blk->sc_read > 0)
      {
        return thinfat_blk_seek(blk, blk, blk->so_current + 1, THINFAT_BLK_EVENT_READ_SINGLE_LOOKUP);
      }
      else
      {
        return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
      }
    }
    else
    {
      printf("Aborting.\n");
      return res == THINFAT_RESULT_ABORT ? THINFAT_RESULT_OK : res;
    }
    break;
  case THINFAT_BLK_EVENT_READ_SINGLE_LOOKUP:
    return thinfat_blk_read_each_sector(blk->client, blk, blk->sc_read, blk->event);
  case THINFAT_BLK_EVENT_READ_CLUSTER_LOOKUP:
    if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
      return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
    else
    {
      thinfat_sector_t si_read = thinfat_ctos(tf, blk->ci_current);
      thinfat_sector_t sc_read = 1 << tf->ctos_shift;
      if (sc_read > blk->sc_read)
        sc_read = blk->sc_read;
      blk->sc_read -= sc_read;
      return thinfat_phy_read_multiple(blk, tf->phy, si_read, sc_read, THINFAT_BLK_EVENT_READ_CLUSTER);
    }
    break;
  case THINFAT_BLK_EVENT_WRITE_CLUSTER_LOOKUP:
    if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
      return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
    else
    {
      thinfat_sector_t si_write = thinfat_ctos(tf, blk->ci_current);
      thinfat_sector_t sc_write = 1 << tf->ctos_shift;
      if (sc_write > blk->sc_write)
        sc_write = blk->sc_write;
      blk->sc_write -= sc_write;
      return thinfat_phy_write_multiple(blk, tf->phy, si_write, sc_write, THINFAT_BLK_EVENT_WRITE_CLUSTER);
    }
    break;
  case THINFAT_BLK_EVENT_READ_CLUSTER:
    if (p_param == NULL)
    {
      if (blk->sc_read > 0)
        return thinfat_blk_seek(blk, blk, ((blk->so_current >> tf->ctos_shift) + 1) << tf->ctos_shift, THINFAT_BLK_EVENT_READ_CLUSTER_LOOKUP);
      else
        return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
    }
    else if (*(void **)p_param == NULL)
    {
      if (blk->next_data != NULL)
      {
        *(void **)p_param = blk->next_data;
        return THINFAT_RESULT_OK;
      }
      return thinfat_core_callback(blk->client, blk->event, s_param, p_param);
    }
    else
    {
      thinfat_result_t res = thinfat_core_callback(blk->client, blk->event, s_param, p_param);
      blk->next_data = *(void **)p_param;
      return res;
    }
    break;
  case THINFAT_BLK_EVENT_WRITE_CLUSTER:
    if (p_param == NULL)
    {
      if (blk->sc_write > 0)
        return thinfat_blk_seek(blk, blk, ((blk->so_current >> tf->ctos_shift) + 1) << tf->ctos_shift, THINFAT_BLK_EVENT_WRITE_CLUSTER_LOOKUP);
    }
    else if (*(void **)p_param == NULL)
    {
      if (blk->next_data != NULL)
      {
        *(void **)p_param = blk->next_data;
        return THINFAT_RESULT_OK;
      }
      return thinfat_core_callback(blk->client, blk->event, s_param, p_param);
    }
    else
    {
      thinfat_result_t res = thinfat_core_callback(blk->client, blk->event, s_param, p_param);
      blk->next_data = *(void **)p_param;
      return res;
    }
    break;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_init(thinfat_blk_t *blk, thinfat_t *parent)
{
  blk->state = THINFAT_BLK_STATE_IDLE;
  blk->cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  blk->parent = parent;
  thinfat_cache_init(blk->cache, parent);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_open(thinfat_blk_t *blk, thinfat_cluster_t ci)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  blk->ci_head = ci;
  blk->ci_current = ci;
  blk->so_current = 0;
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_blk_start_lookup(thinfat_blk_t *blk, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)blk->parent;
  thinfat_sector_t si = tf->si_hidden + tf->sc_reserved + blk->ci_current / (THINFAT_SECTOR_SIZE >> tf->type);

  return thinfat_cached_read_single(blk, tf->cache, si, event);
}

thinfat_result_t thinfat_blk_rewind(thinfat_blk_t *blk)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  blk->ci_current = blk->ci_head;
  blk->so_current = 0;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_seek(void *client, thinfat_blk_t *blk, thinfat_sector_t so_seek, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)blk->parent;

  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  if (blk->ci_current == 0)
  {
    blk->so_current = so_seek;
    return thinfat_core_callback(client, event, so_seek, NULL);
  }
  else
  {
    if (so_seek >> tf->ctos_shift == blk->so_current >> tf->ctos_shift)
    {
      blk->so_current = so_seek;
      return thinfat_core_callback(client, event, so_seek, NULL);
    }
    else if (so_seek >> tf->ctos_shift == 0)
    {
      blk->so_current = so_seek;
      blk->ci_current = blk->ci_head;
      return thinfat_core_callback(client, event, so_seek, NULL);
    }
    else
    {
      if (so_seek < blk->so_current)
      {
        blk->so_current = 0;
        blk->ci_current = blk->ci_head;
      }
      blk->state = THINFAT_BLK_STATE_SEEK;
      blk->seek_event = event;
      blk->seek_client = client;
      blk->so_seek = so_seek;
      return thinfat_blk_start_lookup(blk, THINFAT_BLK_EVENT_LOOKUP);
    }
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_read_each_sector(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_read, thinfat_core_event_t event)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;
  else if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
    return THINFAT_RESULT_EOF;
  else
  {
    thinfat_t *tf = (thinfat_t *)blk->parent;
    thinfat_sector_t si_read;
    if (blk->ci_current >= 2)
      si_read = thinfat_ctos(tf, blk->ci_current) + (blk->so_current & ((1 << tf->ctos_shift) - 1));
    else
      si_read = tf->si_root + blk->so_current;
    blk->state = THINFAT_BLK_STATE_READ;
    blk->event = event;
    blk->sc_read = sc_read;
    blk->client = client;
    blk->next_data = NULL;
    return thinfat_cached_read_single(blk, blk->cache, si_read, THINFAT_BLK_EVENT_READ_SINGLE);
  }
}

thinfat_result_t thinfat_blk_read_each_cluster(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_read, thinfat_core_event_t event)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;
  else if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
    return THINFAT_RESULT_EOF;
  else
  {
    thinfat_t *tf = (thinfat_t *)blk->parent;
    thinfat_sector_t so_cluster = (blk->so_current & ((1 << tf->ctos_shift) - 1));
    thinfat_sector_t si_read = thinfat_ctos(tf, blk->ci_current) + so_cluster;
    thinfat_sector_t sc_stride = (1 << tf->ctos_shift) - so_cluster;
    if (sc_stride > sc_read)
      sc_stride = sc_read;
    blk->state = THINFAT_BLK_STATE_READ;
    blk->event = event;
    blk->sc_read = sc_read - sc_stride;
    blk->client = client;
    blk->next_data = NULL;
    return thinfat_phy_read_multiple(blk, tf->phy, si_read, sc_stride, THINFAT_BLK_EVENT_READ_CLUSTER);
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_write_each_cluster(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_write, thinfat_core_event_t event)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;
  else if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
    return THINFAT_RESULT_EOF;
  else
  {
    thinfat_t *tf = (thinfat_t *)blk->parent;
    thinfat_sector_t so_cluster = (blk->so_current & ((1 << tf->ctos_shift) - 1));
    thinfat_sector_t si_write = thinfat_ctos(tf, blk->ci_current) + so_cluster;
    thinfat_sector_t sc_stride = (1 << tf->ctos_shift) - so_cluster;
    if (sc_stride > sc_write)
      sc_stride = sc_write;
    blk->state = THINFAT_BLK_STATE_WRITE;
    blk->event = event;
    blk->sc_write = sc_write;
    blk->client = client;
    blk->next_data = NULL;
    return thinfat_phy_write_multiple(blk, tf->phy, si_write, sc_stride, THINFAT_BLK_EVENT_WRITE_CLUSTER);
  }
  return THINFAT_RESULT_OK;
}
