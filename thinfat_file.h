/*!
 * @file thinfat_file.h
 * @brief thinFAT FILE layer interface
 * @date 2017/01/18
 * @author Hiroka IHARA
 */
#ifndef THINFAT_FILE_H
#define THINFAT_FILE_H

#include "thinfat_common.h"
#include "thinfat_blk.h"

struct thinfat_tag;
struct thinfat_dir_entry_tag;

typedef struct thinfat_file_tag
{
  void *client;
  struct thinfat_tag *parent;
  thinfat_size_t counter, advance;
  thinfat_off_t position;
  thinfat_size_t size;
  void *buffer;
  thinfat_blk_t blk;
}
thinfat_file_t;

thinfat_result_t thinfat_file_callback(thinfat_file_t *file, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_file_init(thinfat_file_t *file, struct thinfat_tag *parent, struct thinfat_cache_tag *cache);
thinfat_result_t thinfat_file_open(thinfat_file_t *file, const struct thinfat_dir_entry_tag *entry);
thinfat_result_t thinfat_file_read(void *client, thinfat_file_t *file, void *buf, thinfat_size_t size, thinfat_core_event_t event);
thinfat_result_t thinfat_file_write(void *client, thinfat_file_t *file, const void *buf, thinfat_size_t size, thinfat_core_event_t event);

#endif
