/*!
 * @file thinfat_dir.h
 * @brief thinFAT DIR layer interface
 * @date 2017/01/18
 * @author Hiroka IHARA
 */
#ifndef THINFAT_DIR_H
#define THINFAT_DIR_H

#include "thinfat_common.h"
#include "thinfat_blk.h"

struct thinfat_tag;
struct thinfat_cache_tag;

typedef struct thinfat_dir_tag
{
  void *client;
  thinfat_core_event_t event;
  struct thinfat_tag *parent;
  union
  {
    struct
    {
      const void *target_name;
      unsigned int nc_matched;
      uint8_t checksum;
    };
  };
  thinfat_blk_t blk;
}
thinfat_dir_t;

typedef struct thinfat_dir_entry_tag
{
  uint8_t attr;
  uint8_t name[12];
  thinfat_cluster_t ci_head;
  uint32_t size;
}
thinfat_dir_entry_t;

typedef struct thinfat_lfn_entry_tag
{
  uint8_t order;
  uint8_t checksum;
  wchar_t partial_name[14];
}
thinfat_lfn_entry_t;

thinfat_result_t thinfat_dir_callback(thinfat_dir_t *dir, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_dir_init(thinfat_dir_t *dir, struct thinfat_tag *parent, struct thinfat_cache_tag *cache);
thinfat_result_t thinfat_dir_dump(void *client, thinfat_dir_t *dir, thinfat_core_event_t event);
thinfat_result_t thinfat_dir_find(void *client, thinfat_dir_t *dir, const char *name, thinfat_core_event_t event);
thinfat_result_t thinfat_dir_find_by_longname(void *client, thinfat_dir_t *dir, const wchar_t *name, thinfat_core_event_t event);
thinfat_result_t thinfat_dir_open(void *client, thinfat_dir_t *dir, const thinfat_dir_entry_t *entry);

#endif
