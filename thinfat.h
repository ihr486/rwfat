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
struct thinfat_table_tag;
struct thinfat_dir_tag;
struct thinfat_cache_tag;

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
  struct thinfat_dir_tag *cur_dir;
  struct thinfat_table_tag *table;
  struct thinfat_cache_tag *table_cache, *dir_cache, *file_cache;
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
  thinfat_cluster_t cc_free, ci_next_free;
  thinfat_event_t event;
}
thinfat_t;

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param);

static inline uint8_t thinfat_read_u8(const void *p, size_t offset)
{
  return ((const uint8_t *)p)[offset];
}

static inline uint16_t thinfat_read_u16(const void *p, size_t offset)
{
  return ((const uint8_t *)p)[offset] | ((uint16_t)((const uint8_t *)p)[offset + 1] << 8);
}

static inline uint32_t thinfat_read_u24(const void *p, size_t offset)
{
  return ((const uint8_t *)p)[offset] | ((uint32_t)((const uint8_t *)p)[offset + 1] << 8) | ((uint32_t)((const uint8_t *)p)[offset + 2] << 16);
}

static inline uint32_t thinfat_read_u32(const void *p, size_t offset)
{
  return ((const uint8_t *)p)[offset] | ((uint32_t)((const uint8_t *)p)[offset + 1] << 8) | ((uint32_t)((const uint8_t *)p)[offset + 2] << 16) | ((uint32_t)((const uint8_t *)p)[offset + 3] << 24);
}

static inline void thinfat_write_u8(void *p, size_t offset, uint8_t value)
{
  ((uint8_t *)p)[offset] = value;
}

static inline void thinfat_write_u16(void *p, size_t offset, uint16_t value)
{
  ((uint8_t *)p)[offset] = value & 0xFF;
  ((uint8_t *)p)[offset + 1] = (value >> 8) & 0xFF;
}

static inline void thinfat_write_u24(void *p, size_t offset, uint32_t value)
{
  ((uint8_t *)p)[offset] = value & 0xFF;
  ((uint8_t *)p)[offset + 1] = (value >> 8) & 0xFF;
  ((uint8_t *)p)[offset + 2] = (value >> 16) & 0xFF;
}

static inline void thinfat_write_u32(void *p, size_t offset, uint32_t value)
{
  ((uint8_t *)p)[offset] = value & 0xFF;
  ((uint8_t *)p)[offset + 1] = (value >> 8) & 0xFF;
  ((uint8_t *)p)[offset + 2] = (value >> 16) & 0xFF;
  ((uint8_t *)p)[offset + 3] = (value >> 24) & 0xFF;
}

static inline thinfat_cluster_t thinfat_stoc(thinfat_t *tf, thinfat_sector_t si)
{
  return ((si - tf->si_data) >> tf->ctos_shift) + 2;
}

static inline thinfat_sector_t thinfat_ctos(thinfat_t *tf, thinfat_cluster_t ci)
{
  return ((ci - 2) << tf->ctos_shift) + tf->si_data;
}

thinfat_result_t thinfat_core_callback(void *instance, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy);
thinfat_result_t thinfat_finalize(thinfat_t *tf);

thinfat_result_t thinfat_find_partition(thinfat_t *tf, thinfat_event_t event);
thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_sector_t sector, thinfat_event_t event);
thinfat_result_t thinfat_unmount(thinfat_t *tf, thinfat_event_t event);

#endif
