/*!
 * @file thinfat_blk.c
 * @brief thinFAT BLK layer implementation
 * @date 2017/01/08
 * @author Hiroka IHARA
 */
#include "thinfat.h"
#include "thinfat_phy.h"
#include "thinfat_blk.h"
#include "thinfat_table.h"
#include "thinfat_cache.h"

#include <stdlib.h>
#include <string.h>

thinfat_result_t thinfat_blk_callback(thinfat_blk_t *blk, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)blk->parent;
  thinfat_result_t res;
  blk->state = THINFAT_BLK_STATE_IDLE;
  switch(event)
  {
  case THINFAT_BLK_EVENT_READ_SINGLE:
    if ((res = thinfat_core_callback(blk->client, blk->event, s_param, *(void **)p_param)) == THINFAT_RESULT_OK)
    {
      if (--blk->sc_read > 0)
      {
        return thinfat_table_seek(blk, tf->table, blk->so_current + 1, THINFAT_BLK_EVENT_READ_SINGLE_LOOKUP);
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
    if (blk->ci_current >= 2)
    {
      thinfat_sector_t si_read = thinfat_ctos(tf, blk->ci_current) + (blk->so_current & ((1 << tf->ctos_shift) - 1));
      
      return thinfat_cached_read_single(blk, blk->cache, si_read, THINFAT_BLK_EVENT_READ_SINGLE);
    }
    else
      return thinfat_cached_read_single(blk, blk->cache, tf->si_root + blk->so_current, THINFAT_BLK_EVENT_READ_SINGLE);
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
        return thinfat_table_seek(blk, tf->table, ((blk->so_current >> tf->ctos_shift) + 1) << tf->ctos_shift, THINFAT_BLK_EVENT_READ_CLUSTER_LOOKUP);
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
        return thinfat_table_seek(blk, tf->table, ((blk->so_current >> tf->ctos_shift) + 1) << tf->ctos_shift, THINFAT_BLK_EVENT_WRITE_CLUSTER_LOOKUP);
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

thinfat_result_t thinfat_blk_open(thinfat_blk_t *blk, const thinfat_dir_entry_t *entry)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  blk->ci_head = entry->ci_head;
  blk->ci_current = entry->ci_head;
  blk->so_current = 0;
  blk->byte_pointer = 0;
  blk->file_size = entry->size;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_open_dir(thinfat_blk_t *blk, thinfat_cluster_t ci)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  blk->ci_head = ci;
  blk->ci_current = ci;
  blk->so_current = 0;
  blk->byte_pointer = 0;
  blk->file_size = 0;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_rewind(thinfat_blk_t *blk)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;

  blk->ci_current = blk->ci_head;
  blk->so_current = 0;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_blk_read_each_sector(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_read, thinfat_core_event_t event)
{
  if (blk->state != THINFAT_BLK_STATE_IDLE)
    return THINFAT_RESULT_BLK_BUSY;
  else if (!THINFAT_IS_CLUSTER_VALID(blk->ci_current))
    return thinfat_core_callback(blk->client, blk->event, THINFAT_INVALID_SECTOR, NULL);
  else
  {
    thinfat_t *tf = (thinfat_t *)blk->parent;
    blk->state = THINFAT_BLK_STATE_READ;
    blk->event = event;
    blk->sc_read = sc_read;
    blk->client = client;
    blk->next_data = NULL;
    //return thinfat_cached_read_single(blk, blk->cache, si_read, THINFAT_BLK_EVENT_READ_SINGLE);
    return thinfat_table_seek(blk, tf->table, blk->so_current, THINFAT_BLK_EVENT_READ_SINGLE_LOOKUP);
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
    //return thinfat_phy_read_multiple(blk, tf->phy, si_read, sc_stride, THINFAT_BLK_EVENT_READ_CLUSTER);
    return thinfat_table_seek(blk, tf->table, blk->so_current, THINFAT_BLK_EVENT_READ_CLUSTER_LOOKUP);
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
    //return thinfat_phy_write_multiple(blk, tf->phy, si_write, sc_stride, THINFAT_BLK_EVENT_WRITE_CLUSTER);
    return thinfat_table_seek(blk, tf->table, blk->so_current, THINFAT_BLK_EVENT_WRITE_CLUSTER_LOOKUP);
  }
  return THINFAT_RESULT_OK;
}
