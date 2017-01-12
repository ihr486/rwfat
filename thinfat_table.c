#include "thinfat.h"
#include "thinfat_table.h"
#include "thinfat_cache.h"

thinfat_result_t thinfat_table_callback(thinfat_table_t *table, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  thinfat_t *tf = (thinfat_t *)table->parent;
  thinfat_cluster_t ci_next = THINFAT_INVALID_CLUSTER;
  table->state = THINFAT_TABLE_STATE_IDLE;
  switch(event)
  {
  case THINFAT_TABLE_EVENT_LOOKUP:
    if (tf->type == THINFAT_TYPE_FAT32)
    {
      ci_next = thinfat_read_u32(*(void **)p_param, (table->ci_target * 4) % THINFAT_SECTOR_SIZE);
      printf("Next cluster = 0x%08X\n", blk->ci_next);
    }
    else
    {
      ci_next = thinfat_read_u16(*(void **)p_param, (table->ci_target * 2) % THINFAT_SECTOR_SIZE);
      if (ci_next >= 0xFFF7)
      {
        ci_next = THINFAT_INVALID_CLUSTER;
      }
    }
    return thinfat_core_callback(table->client, table->event, THINFAT_INVALID_SECTOR, &ci_next);
  }
}

thinfat_result_t thinfat_table_init(thinfat_table_t *table, thinfat_t *parent)
{
  table->parent = parent;
  table->state = THINFAT_TABLE_STATE_IDLE;
  table->cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(table->cache, parent);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_table_lookup(void *client, thinfat_table_t *table, thinfat_cluster_t ci_target, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)table->parent;
  thinfat_sector_t si = tf->si_hidden + tf->sc_reserved + ci_target / (THINFAT_SECTOR_SIZE >> tf->type);
  table->client = client;
  table->event = event;
  table->ci_target = ci_target;
  return thinfat_cached_read_single(table, table->cache, si, THINFAT_TABLE_EVENT_LOOKUP);
}
