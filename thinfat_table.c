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
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_table_init(thinfat_table_t *table, thinfat_t *parent)
{
  table->parent = parent;
  table->state = THINFAT_TABLE_STATE_IDLE;
  table->cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(table->cache, parent);
  return THINFAT_RESULT_OK;
}
