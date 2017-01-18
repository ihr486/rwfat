/*!
 * @file thinfat_table.h
 * @brief thinFAT TBL layer interface
 * @date 2017/01/13
 * @author Hiroka IHARA
 */
#ifndef THINFAT_TABLE_H
#define THINFAT_TABLE_H

#include "thinfat_common.h"

struct thinfat_tag;
struct thinfat_cache_tag;

typedef struct thinfat_table_tag
{
  void *client;
  struct thinfat_tag *parent;
  thinfat_core_event_t event;
  struct thinfat_cache_tag *cache;
  union
  {
    struct
    {
      thinfat_cluster_t ci_current;
      thinfat_cluster_t cc_seek;
    };
    struct
    {
      thinfat_sector_t so_seek;
    };
    struct
    {
      thinfat_cluster_t cc_search_count;
      thinfat_cluster_t cc_search;
    };
    struct
    {
      thinfat_cluster_t ci_from;
      thinfat_cluster_t ci_to;
    };
  };
}
thinfat_table_t;

thinfat_result_t thinfat_table_callback(thinfat_table_t *table, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_table_init(thinfat_table_t *table, thinfat_t *parent, struct thinfat_cache_tag *cache);
thinfat_result_t thinfat_table_seek(void *client, thinfat_table_t *table, thinfat_sector_t so_seek, thinfat_core_event_t event);
thinfat_result_t thinfat_table_allocate(void *client, thinfat_table_t *table, thinfat_cluster_t cc_alloc, thinfat_core_event_t event);
thinfat_result_t thinfat_table_deallocate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_dealloc, thinfat_cluster_t cc_dealloc, thinfat_core_event_t event);
thinfat_result_t thinfat_table_concatenate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_from, thinfat_cluster_t ci_to, thinfat_core_event_t event);
thinfat_result_t thinfat_table_truncate(void *client, thinfat_table_t *table, thinfat_cluster_t ci_from, thinfat_core_event_t event);

#endif
