#ifndef THINFAT_H
#define THINFAT_H

#include "thinfat_common.h"

#define THINFAT_TABLE_CACHE(tf) ((tf)->table_cache)
#define THINFAT_DATA_CACHE(tf) ((tf)->data_cache)

struct thinfat_tag;
typedef void (*thinfat_user_callback_t)(struct thinfat_tag *tf);

typedef uint8_t thinfat_cache_t[THINFAT_SECTOR_SIZE];

typedef enum
{
  THINFAT_STATE_IDLE = 0
}
thinfat_state_t;

struct thinfat_phy_tag;

typedef enum
{
    THINFAT_TYPE_UNKNOWN = 0,
    THINFAT_TYPE_FAT16,
    THINFAT_TYPE_FAT32
}
thinfat_type_t;

typedef struct
{
  thinfat_type_t type;
  thinfat_state_t state;
  struct thinfat_phy_tag *phy;
  thinfat_cache_t table_cache;
  thinfat_cache_t data_cache;
  uint8_t cluster_shift;
  uint8_t table_redundancy;
  thinfat_sector_t sc_reserved;
  uint16_t root_entry_count;
  thinfat_sector_t sc_volume_size;
  thinfat_sector_t sc_table_size;
  thinfat_cluster_t ci_root;
  thinfat_sector_t si_data;
  thinfat_sector_t si_hidden;
  thinfat_cluster_t ci_current_dir;
  thinfat_user_callback_t callback;
}
thinfat_t;

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

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy);
thinfat_result_t thinfat_finalize(thinfat_t *tf);

thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_user_callback_t callback);
thinfat_result_t thinfat_unmount(thinfat_t *tf);

#endif
