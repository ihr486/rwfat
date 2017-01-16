/*!
 * @file thinfat_table.c
 * @brief thinFAT TBL layer implementation
 * @date 2017/01/13
 * @author Hiroka IHARA
 */
#include "thinfat.h"
#include "thinfat_blk.h"
#include "thinfat_table.h"
#include "thinfat_cache.h"

#include <stdlib.h>

thinfat_result_t thinfat_table_seek(void *client, thinfat_table_t *table, thinfat_sector_t so_seek, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)table->parent;
  thinfat_blk_t *blk = (thinfat_blk_t *)client;

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
      if (table->state != THINFAT_TABLE_STATE_IDLE)
        return THINFAT_RESULT_TABLE_BUSY;

      if (so_seek < blk->so_current)
      {
        blk->so_current = 0;
        blk->ci_current = blk->ci_head;
      }
      thinfat_sector_t si_lookup = tf->si_hidden + tf->sc_reserved + blk->ci_current / (THINFAT_SECTOR_SIZE >> tf->type);
      table->so_seek = so_seek;
      table->client = client;
      table->event = event;
      return thinfat_cached_read_single(table, table->cache, si_lookup, THINFAT_TABLE_EVENT_LOOKUP);
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_table_search_read_callback(thinfat_table_t *table, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_table_callback(thinfat_table_t *table, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)table->parent;
  thinfat_blk_t *blk = (thinfat_blk_t *)table->client;
  table->state = THINFAT_TABLE_STATE_IDLE;
  switch(event)
  {
  case THINFAT_TABLE_EVENT_LOOKUP:
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
    blk->so_current += 1 << tf->ctos_shift;
    if (THINFAT_IS_CLUSTER_VALID(blk->ci_current))
      return thinfat_table_seek(table->client, table, table->so_seek, table->event);
    else
      return thinfat_core_callback(table->client, table->event, blk->so_current, NULL);
  case THINFAT_TABLE_EVENT_SEARCH_READ:
    return thinfat_table_search_read_callback(table, s_param, p_param);
  case THINFAT_TABLE_EVENT_SEARCH_FOUND:
    THINFAT_INFO("Cluster found @ " TFF_X32 " * " TFF_U32 "\n", *(thinfat_cluster_t *)p_param, table->cc_search);
    return thinfat_core_callback(table->client, table->event, THINFAT_INVALID_SECTOR, NULL);
  case THINFAT_TABLE_EVENT_CONCATENATE_READ:
    if (tf->type == THINFAT_TYPE_FAT32)
      thinfat_write_u32(*(void **)p_param, (table->ci_from % (THINFAT_SECTOR_SIZE / 4)) * 4, table->ci_to);
    else
      thinfat_write_u16(*(void **)p_param, (table->ci_from % (THINFAT_SECTOR_SIZE / 2)) * 2, table->ci_to);
    return thinfat_core_callback(table->client, table->event, THINFAT_INVALID_SECTOR, NULL);
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_table_search_read_callback(thinfat_table_t *table, thinfat_sector_t si_read, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)table->parent;
  thinfat_cluster_t cc_total = (tf->si_hidden + tf->sc_volume_size - tf->si_data) >> tf->ctos_shift;
  thinfat_cluster_t ci_current = (si_read - tf->si_hidden - tf->sc_reserved) * (THINFAT_SECTOR_SIZE >> tf->type);
  for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE >> tf->type; i++)
  {
    if (ci_current + i >= cc_total + 2)
      return thinfat_core_callback(table->client, table->event, THINFAT_INVALID_SECTOR, NULL);

    thinfat_cluster_t ci_value;
    if (tf->type == THINFAT_TYPE_FAT32)
      ci_value = thinfat_read_u32(*(void **)p_param, i * 4) & THINFAT_FAT32_CLUSTER_MASK;
    else
      ci_value = thinfat_read_u16(*(void **)p_param, i * 2);
    if (ci_value == 0)
    {
      table->cc_search_count++;
      if (table->cc_search_count == table->cc_search)
      {
        thinfat_cluster_t ci_start = ci_current - table->cc_search_count + 1;
        return thinfat_core_callback(table, THINFAT_TABLE_EVENT_SEARCH_FOUND, THINFAT_INVALID_SECTOR, &ci_start);
      }
    }
    else
    {
      table->cc_search_count = 0;
    }
  }
  if (si_read + 1 < tf->sc_table_size)
    return thinfat_cached_read_single(table, table->cache, si_read + 1, THINFAT_TABLE_EVENT_SEARCH_READ);
  else
    return thinfat_core_callback(table->client, table->event, THINFAT_INVALID_SECTOR, NULL);
}

thinfat_result_t thinfat_table_init(thinfat_table_t *table, thinfat_t *parent)
{
  table->parent = parent;
  table->state = THINFAT_TABLE_STATE_IDLE;
  table->cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(table->cache, parent);
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_table_search(thinfat_table_t *table, thinfat_cluster_t cc_search)
{
  thinfat_t *tf = (thinfat_t *)table->parent;

  thinfat_cluster_t cc_current = 0;
  thinfat_cluster_t ci_current = tf->ci_next_free;
  if (!THINFAT_IS_CLUSTER_VALID(ci_current))
    ci_current = 2;
  
  thinfat_cluster_t cc_total = tf->si_hidden + tf->sc_volume_size - tf->si_data;

  thinfat_sector_t si_table = tf->si_hidden + tf->sc_reserved + ci_current / (THINFAT_SECTOR_SIZE >> tf->type);

  table->cc_search = cc_search;
  table->cc_search_count = 0;

  return thinfat_cached_read_single(table, table->cache, si_table, THINFAT_TABLE_EVENT_SEARCH_READ);
}

thinfat_result_t thinfat_table_allocate(void *client, thinfat_table_t *table, thinfat_cluster_t cc_allocate, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)table->parent;

  table->client = client;
  table->event = event;

  return thinfat_table_search(table, cc_allocate);
}

thinfat_result_t thinfat_table_deallocate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_deallocate, thinfat_cluster_t cc_deallocate, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)table->parent;

  table->client = client;
  table->event = event;

  return THINFAT_RESULT_UNSUPPORTED;
}

thinfat_result_t thinfat_table_concatenate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_from, thinfat_cluster_t ci_to, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)table->parent;

  thinfat_sector_t si_read = tf->si_hidden + tf->sc_reserved + ci_from / (THINFAT_SECTOR_SIZE >> tf->type);

  table->client = client;
  table->event = event;
  table->ci_from = ci_from;
  table->ci_to = ci_to;

  return thinfat_cached_read_single(table, table->cache, si_read, THINFAT_TABLE_EVENT_CONCATENATE_READ);
}

thinfat_result_t thinfat_table_truncate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_from, thinfat_core_event_t event)
{
  return thinfat_table_concatenate(client, table, ci_from, THINFAT_FAT32_EOC, event);
}
