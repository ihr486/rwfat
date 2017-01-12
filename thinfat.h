/*!
 * @file thinfat.h
 * @brief thinFAT base public interface
 * @date 2017/01/08
 * @author Hiroka IHARA
 */

#ifndef THINFAT_H
#define THINFAT_H

#include "thinfat_common.h"
#include "thinfat_config.h"

#define THINFAT_TABLE_CACHE(tf) ((tf)->table_cache)
#define THINFAT_DATA_CACHE(tf) ((tf)->data_cache)
#define THINFAT_DIR_CACHE(tf) ((tf)->dir_cache)
#define THINFAT_SI_TABLE_CACHE(tf) ((tf)->si_table_cache)
#define THINFAT_SI_DATA_CACHE(tf) ((tf)->si_data_cache)
#define THINFAT_SI_DIR_CACHE(tf) ((tf)->si_dir_cache)

typedef enum
{
  THINFAT_STATE_IDLE = 0
}
thinfat_state_t;

struct thinfat_phy_tag;
struct thinfat_blk_tag;
struct thinfat_table_tag;

typedef enum
{
  THINFAT_TYPE_UNKNOWN = 0,
  THINFAT_TYPE_FAT16 = 1,
  THINFAT_TYPE_FAT32 = 2
}
thinfat_type_t;

typedef struct thinfat_tag
{
  thinfat_type_t type;
  thinfat_state_t state;
  struct thinfat_phy_tag *phy;
  struct thinfat_blk_tag *cur_file, *cur_dir;
  struct thinfat_table_tag *table;
  uint8_t ctos_shift;
  uint8_t table_redundancy;
  thinfat_sector_t sc_reserved;
  uint16_t root_entry_count;
  thinfat_sector_t sc_volume_size;
  thinfat_sector_t sc_table_size;
  thinfat_cluster_t ci_root;
  thinfat_sector_t si_data;
  thinfat_sector_t si_hidden;
  thinfat_sector_t si_root;
  thinfat_event_t event;
  union
  {
    struct
    {
      const void *target_name;
      int nc_matched;
      uint8_t checksum;
    };
  };
}
thinfat_t;

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

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param);

static inline uint8_t thinfat_read_u8(void *p, size_t offset)
{
  return ((uint8_t *)p)[offset];
}

static inline uint16_t thinfat_read_u16(void *p, size_t offset)
{
  return ((uint8_t *)p)[offset] | ((uint16_t)((uint8_t *)p)[offset + 1] << 8);
}

static inline uint32_t thinfat_read_u24(void *p, size_t offset)
{
  return ((uint8_t *)p)[offset] | ((uint32_t)((uint8_t *)p)[offset + 1] << 8) | ((uint32_t)((uint8_t *)p)[offset + 2] << 16);
}

static inline uint32_t thinfat_read_u32(void *p, size_t offset)
{
  return ((uint8_t *)p)[offset] | ((uint32_t)((uint8_t *)p)[offset + 1] << 8) | ((uint32_t)((uint8_t *)p)[offset + 2] << 16) | ((uint32_t)((uint8_t *)p)[offset + 3] << 24);
}

static inline thinfat_cluster_t thinfat_stoc(thinfat_t *tf, thinfat_sector_t si)
{
  return ((si - tf->si_data) >> tf->ctos_shift) + 2;
}

static inline thinfat_sector_t thinfat_ctos(thinfat_t *tf, thinfat_cluster_t ci)
{
  return ((ci - 2) << tf->ctos_shift) + tf->si_data;
}

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy);
thinfat_result_t thinfat_finalize(thinfat_t *tf);

thinfat_result_t thinfat_find_partition(thinfat_t *tf, thinfat_event_t event);
thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_sector_t sector, thinfat_event_t event);
thinfat_result_t thinfat_unmount(thinfat_t *tf, thinfat_event_t event);
thinfat_result_t thinfat_dump_current_directory(thinfat_t *tf, thinfat_event_t event);
thinfat_result_t thinfat_find_file(thinfat_t *tf, const char *name, thinfat_event_t event);
thinfat_result_t thinfat_find_file_by_longname(thinfat_t *tf, const wchar_t *longname, thinfat_event_t event);
thinfat_result_t thinfat_chdir(thinfat_t *tf, thinfat_cluster_t ci);
thinfat_result_t thinfat_open_file(thinfat_t *tf, thinfat_cluster_t ci);
thinfat_result_t thinfat_read_file(thinfat_t *tf, void *buf, size_t size, thinfat_event_t event);
thinfat_result_t thinfat_write_file(thinfat_t *tf, const void *buf, size_t size, thinfat_event_t event);

#endif
