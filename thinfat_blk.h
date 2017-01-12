/*!
 * @file thinfat_blk.h
 * @brief thinFAT BLK layer interface
 * @date 2017/01/08
 * @author Hiroka IHARA
 */
#ifndef THINFAT_BLK_H
#define THINFAT_BLK_H

#include "thinfat_common.h"

typedef enum
{
  THINFAT_BLK_STATE_IDLE = 0,
  THINFAT_BLK_STATE_SEEK,
  THINFAT_BLK_STATE_READ,
  THINFAT_BLK_STATE_WRITE
}
thinfat_blk_state_t;

struct thinfat_tag;
struct thinfat_cache_tag;

typedef struct thinfat_blk_tag
{
  void *client, *seek_client;
  struct thinfat_tag *parent;
  thinfat_blk_state_t state;
  thinfat_cluster_t ci_head;
  thinfat_cluster_t ci_current;
  thinfat_sector_t so_current;
  size_t byte_pointer;
  thinfat_core_event_t event, seek_event;
  struct thinfat_cache_tag *cache;
  void *next_data;
  thinfat_sector_t so_seek;
  union
  {
    thinfat_sector_t sc_read;
    thinfat_sector_t sc_write;
  };
}
thinfat_blk_t;

thinfat_result_t thinfat_blk_callback(thinfat_blk_t *blk, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);
thinfat_result_t thinfat_blk_init(thinfat_blk_t *blk, struct thinfat_tag *parent);
thinfat_result_t thinfat_blk_open(thinfat_blk_t *blk, thinfat_cluster_t ci);
thinfat_result_t thinfat_blk_rewind(thinfat_blk_t *blk);
thinfat_result_t thinfat_blk_seek(void *client, thinfat_blk_t *blk, thinfat_sector_t so_seek, thinfat_core_event_t event);
thinfat_result_t thinfat_blk_read_each_sector(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_read, thinfat_core_event_t event);
thinfat_result_t thinfat_blk_read_each_cluster(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_read, thinfat_core_event_t event);
thinfat_result_t thinfat_blk_write_each_cluster(void *client, thinfat_blk_t *blk, thinfat_sector_t sc_write, thinfat_core_event_t event);

#endif
