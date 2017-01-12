/*!
 * @file thinfat.c
 * @brief thinFAT core implementation
 * @date 2017/01/07
 * @author Hiroka IHARA
 */
#include "thinfat.h"
#include "thinfat_phy.h"
#include "thinfat_cache.h"
#include "thinfat_blk.h"

#include <stdlib.h>
#include <string.h>

#define THINFAT_ATTR_READ_ONLY (0x01)
#define THINFAT_ATTR_HIDDEN (0x02)
#define THINFAT_ATTR_SYSTEM (0x04)
#define THINFAT_ATTR_VOLUME_ID (0x08)
#define THINFAT_ATTR_DIRECTORY (0x10)
#define THINFAT_ATTR_ARCHIVE (0x20)
#define THINFAT_ATTR_LONG_FILE_NAME (0x0F)

static thinfat_result_t thinfat_read_mbr_callback(thinfat_t *tf, void *mbr);
static thinfat_result_t thinfat_read_parameter_block_callback(thinfat_t *tf, void *bpb);
static thinfat_result_t thinfat_dump_dir_callback(thinfat_t *tf, void *entries);
static thinfat_result_t thinfat_find_file_by_longname_callback(thinfat_t *tf, void *entries);
static thinfat_result_t thinfat_find_file_callback(thinfat_t *tf, void *entries);
static thinfat_result_t thinfat_read_file_callback(thinfat_t *tf, thinfat_sector_t s_param, void *p_param);
static thinfat_result_t thinfat_write_file_callback(thinfat_t *tf, thinfat_sector_t s_param, void *p_param);

static inline thinfat_sector_t thinfat_root_sector(thinfat_t *tf)
{
  return tf->si_hidden + tf->sc_reserved + tf->table_redundancy * tf->sc_table_size;
}

static size_t thinfat_wstrlen(const wchar_t *str)
{
  size_t ret;
  for (ret = 0; str[ret] != L'\0'; ret++);
  return ret;
}

static void thinfat_memcpy(void *dest, const void *src, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
  }
}

static inline thinfat_sector_t thinfat_root_sector_count(thinfat_t *tf)
{
  return (tf->root_entry_count * 32 + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
}

thinfat_result_t thinfat_core_callback(void *instance, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  //THINFAT_INFO("Core callback: %p, %d, " TFF_X32 ", %p\n", instance, event, s_param, p_param);
  if (event < THINFAT_CORE_EVENT_MAX)
  {
    switch(event)
    {
    case THINFAT_CORE_EVENT_READ_MBR:
      return thinfat_read_mbr_callback((thinfat_t *)instance, *(void **)p_param);
    case THINFAT_CORE_EVENT_READ_BPB:
      return thinfat_read_parameter_block_callback((thinfat_t *)instance, *(void **)p_param);
    case THINFAT_CORE_EVENT_DUMP_DIR:
      return thinfat_dump_dir_callback((thinfat_t *)instance, p_param);
    case THINFAT_CORE_EVENT_FIND_FILE:
      return thinfat_find_file_callback((thinfat_t *)instance, p_param);
    case THINFAT_CORE_EVENT_FIND_FILE_BY_LONGNAME:
      return thinfat_find_file_by_longname_callback((thinfat_t *)instance, p_param);
    case THINFAT_CORE_EVENT_READ_FILE:
      return thinfat_read_file_callback((thinfat_t *)instance, s_param, p_param);
    case THINFAT_CORE_EVENT_WRITE_FILE:
      return thinfat_write_file_callback((thinfat_t *)instance, s_param, p_param);
    }
  }
  else if (event < THINFAT_CACHE_EVENT_MAX)
  {
    return thinfat_cache_callback((thinfat_cache_t *)instance, event, s_param, p_param);
  }
  else if (event < THINFAT_BLK_EVENT_MAX)
  {
    return thinfat_blk_callback((thinfat_blk_t *)instance, event, s_param, p_param);
  }
  else if (event < THINFAT_TABLE_EVENT_MAX)
  {
    return thinfat_table_callback((thinfat_table_t *)instance, event, s_param, p_param);
  }
  else
  {
    return thinfat_user_callback((thinfat_t *)instance, event, s_param, p_param);
  }
  return THINFAT_RESULT_OK;
}

static thinfat_type_t thinfat_determine_type(const thinfat_t *tf)
{
  thinfat_cluster_t cluster_count = (tf->sc_volume_size - tf->si_data) >> tf->ctos_shift;
  if (cluster_count < 4085)
  {
    return THINFAT_TYPE_UNKNOWN;
  }
  else if (cluster_count < 65525)
  {
    return THINFAT_TYPE_FAT16;
  }
  return THINFAT_TYPE_FAT32;
}

static thinfat_result_t thinfat_read_parameter_block_callback(thinfat_t *tf, void *bpb)
{
  uint16_t signature = thinfat_read_u16(bpb, 510);
  if (signature != 0xAA55)
  {
    THINFAT_ERROR("Corrupt BPB signature detected.\n");
    return THINFAT_RESULT_ERROR_SIGNATURE;
  }

  uint16_t BPB_BytsPerSec = thinfat_read_u16(bpb, 11);
  if (BPB_BytsPerSec != THINFAT_SECTOR_SIZE)
  {
      THINFAT_ERROR("Illegal sector size: %u(read) != %u(configured).\n", BPB_BytsPerSec, THINFAT_SECTOR_SIZE);
      return THINFAT_RESULT_ERROR_BPB;
  }

  uint8_t BPB_SecPerClus = thinfat_read_u8(bpb, 13);
  for (tf->ctos_shift = 0; !(BPB_SecPerClus & 1); BPB_SecPerClus >>= 1)
  {
      tf->ctos_shift++;
  }
  if (BPB_SecPerClus != 1)
  {
      THINFAT_ERROR("BPB_SecPerClus is not a power of 2.\n");
      return THINFAT_RESULT_ERROR_BPB;
  }

  tf->sc_reserved = thinfat_read_u16(bpb, 14);
  THINFAT_INFO("Reserved region: %u sectors\n", tf->sc_reserved);

  tf->table_redundancy = thinfat_read_u8(bpb, 16);
  THINFAT_INFO("Number of FATs: %u\n", tf->table_redundancy);

  tf->root_entry_count = thinfat_read_u16(bpb, 17);
  THINFAT_INFO("Number of entries in root directory: %u\n", tf->root_entry_count);

  tf->sc_volume_size = thinfat_read_u16(bpb, 19);
  THINFAT_INFO("Number of sectors(16bit): %u\n", tf->sc_volume_size);

  tf->sc_table_size = thinfat_read_u16(bpb, 22);
  THINFAT_INFO("FAT size(16bit): %u sectors\n", tf->sc_table_size);

  if (tf->sc_volume_size == 0)
  {
      tf->sc_volume_size = thinfat_read_u32(bpb, 32);
      THINFAT_INFO("Number of sectors(32bit): %u\n", tf->sc_volume_size);
  }

  if (tf->sc_table_size == 0)
  {
      tf->sc_table_size = thinfat_read_u32(bpb, 36);
      THINFAT_INFO("FAT size(32bit): %u sectors\n", tf->sc_table_size);
  }

  tf->si_data = tf->si_hidden + tf->sc_reserved + tf->sc_table_size * tf->table_redundancy + (tf->root_entry_count * 32 + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;

  tf->type = thinfat_determine_type(tf);

  if (tf->type == THINFAT_TYPE_FAT32)
    tf->ci_root = thinfat_read_u32(bpb, 44);
  else
    tf->ci_root = 0;
  THINFAT_INFO("Root cluster: %u\n", tf->ci_root);

  tf->si_root = tf->si_hidden + tf->sc_reserved + tf->sc_table_size * tf->table_redundancy;

  thinfat_blk_open(tf->cur_dir, tf->ci_root);

  return thinfat_core_callback(tf, tf->event, tf->si_hidden, NULL);
}

static thinfat_result_t thinfat_read_mbr_callback(thinfat_t *tf, void *mbr)
{
  uint16_t signature = thinfat_read_u16(mbr, 510);
  if (signature != 0xAA55)
  {
    THINFAT_ERROR("Corrupt MBR signature detected.\n");
    return THINFAT_RESULT_ERROR_SIGNATURE;
  }

  for (unsigned int i = 0; i < 4; i++)
  {
    uint8_t partition_type = thinfat_read_u8(mbr, 446 + i * 16 + 4);
    thinfat_sector_t partition_offset = thinfat_read_u32(mbr, 446 + i * 16 + 8);

    switch (partition_type)
    {
    case 0x0B:
    case 0x0C:
      THINFAT_INFO("Partition %u@" TFF_X32 ": FAT32(LBA)\n", i, partition_offset);
      return thinfat_core_callback(tf, tf->event, partition_offset, NULL);
    case 0x04:
    case 0x06:
    case 0x0E:
      THINFAT_INFO("Partition %u@" TFF_X32 ": FAT16(LBA)\n", i, partition_offset);
      return thinfat_core_callback(tf, tf->event, partition_offset, NULL);
    }
  }
  THINFAT_ERROR("No partition found on the disk.\n");
  return THINFAT_RESULT_NO_PARTITION;
}

static thinfat_dir_entry_t *thinfat_decode_dir_entry(uint8_t *src, thinfat_dir_entry_t *dest)
{
  dest->attr = thinfat_read_u8(src, 11);
  memcpy(dest->name, src, 11);
  dest->name[11] = '\0';
  dest->ci_head = ((uint32_t)thinfat_read_u16(src, 20) << 16) | thinfat_read_u16(src, 26);
  dest->size = thinfat_read_u32(src, 28);
  return dest;
}

static thinfat_lfn_entry_t *thinfat_decode_lfn_entry(uint8_t *src, thinfat_lfn_entry_t *dest)
{
  dest->order = src[0];
  dest->checksum = src[13];
  for (int i = 0; i < 5; i++)
    dest->partial_name[i] = thinfat_read_u16(src, i * 2 + 1);
  for (int i = 0; i < 6; i++)
    dest->partial_name[i + 5] = thinfat_read_u16(src, i * 2 + 14);
  for (int i = 0; i < 2; i++)
    dest->partial_name[i + 11] = thinfat_read_u16(src, i * 2 + 28);
  dest->partial_name[13] = L'\0';
  return dest;
}

static thinfat_result_t thinfat_find_file_by_longname_callback(thinfat_t *tf, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0xE5)
      {
        if (entry.attr == THINFAT_ATTR_LONG_FILE_NAME)
        {
          thinfat_lfn_entry_t lfn;
          thinfat_decode_lfn_entry((uint8_t *)entries + i * 32, &lfn);
          if (lfn.order & 0x40)
            tf->nc_matched = 0;
          else if (lfn.checksum != tf->checksum)
            tf->nc_matched = 0;
          else if (tf->nc_matched > 255)
            tf->nc_matched = 0;
          unsigned int ic_match = 13 * (entry.name[0] & 0x1F) - 13;
          unsigned int j;
          const wchar_t *target_name = (const wchar_t *)tf->target_name;
          unsigned int nc_target = thinfat_wstrlen(target_name);
          for (j = 0; j < 13 && ic_match + j < nc_target; j++)
          {
            if (target_name[ic_match + j] != lfn.partial_name[j]) break;
          }
          tf->nc_matched += j;
          if (tf->nc_matched == nc_target)
          {
            tf->nc_matched = 256;
          }
          tf->checksum = lfn.checksum;
        }
        else if (!(entry.attr & (THINFAT_ATTR_HIDDEN | THINFAT_ATTR_SYSTEM)) && entry.name[0] != 0x05)
        {
          if (tf->nc_matched > 255)
          {
            tf->nc_matched = 0;
            thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, &entry);
            return THINFAT_RESULT_ABORT;
          }
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_find_file_callback(thinfat_t *tf, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0x05 && entry.name[0] != 0xE5)
      {
        if (!(entry.attr & (THINFAT_ATTR_HIDDEN | THINFAT_ATTR_SYSTEM)))
        {
          unsigned int j;
          for (j = 0; j < 11; j++)
          {
            if (entry.name[j] != ((const char *)tf->target_name)[j]) break;
          }
          if (j == 11)
          {
            thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, &entry);
            return THINFAT_RESULT_ABORT;
          }
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_dump_dir_callback(thinfat_t *tf, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0xE5)
      {
        if (entry.attr == THINFAT_ATTR_LONG_FILE_NAME)
        {
          if (entry.name[0] & 0x40)
          {
            THINFAT_INFO("LFN[%d] (LAST)\n", entry.name[0] & 0x3F);
          }
          else
          {
            THINFAT_INFO("LFN[%d]\n", entry.name[0]);
          }
        }
        else if (entry.name[0] != 0x05)
        {
          THINFAT_INFO("\"%s\" @ " TFF_X32 " * " TFF_U32 "\n", entry.name, entry.ci_head, entry.size);
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_read_file_callback(thinfat_t *tf, thinfat_sector_t s_param, void *p_param)
{
  if (p_param == NULL)
  {
    tf->cur_file->byte_pointer += tf->cur_file->byte_advance;
    return thinfat_core_callback(tf, tf->event, s_param, p_param);
  }
  else if (*(void **)p_param == NULL)
  {
    *(void **)p_param = tf->cur_file->cache->data;
  }
  else
  {
  }
  THINFAT_INFO("Read callback @ " TFF_X32 " = %p\n", s_param, *(void **)p_param);
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_write_file_callback(thinfat_t *tf, thinfat_sector_t s_param, void *p_param)
{
  THINFAT_INFO("Write callback @ " TFF_X32 " = %p\n", s_param, *(void **)p_param);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_find_partition(thinfat_t *tf, thinfat_event_t event)
{
  tf->event = event;
  return thinfat_cached_read_single(tf, tf->table->cache, 0, THINFAT_CORE_EVENT_READ_MBR);
}

thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_sector_t sector, thinfat_event_t event)
{
  tf->event = event;
  tf->si_hidden = sector;
  return thinfat_cached_read_single(tf, tf->table->cache, sector, THINFAT_CORE_EVENT_READ_BPB);
}

thinfat_result_t thinfat_unmount(thinfat_t *tf, thinfat_event_t event)
{
  tf->event = event;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy)
{
  tf->phy = phy;

  tf->table = (thinfat_table_t *)malloc(sizeof(thinfat_table_t));
  thinfat_table_init(tf->table, tf);

  tf->cur_file = (thinfat_blk_t *)malloc(sizeof(thinfat_blk_t));
  thinfat_blk_init(tf->cur_file, tf);

  tf->cur_dir = (thinfat_blk_t *)malloc(sizeof(thinfat_blk_t));
  thinfat_blk_init(tf->cur_dir, tf);

  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_finalize(thinfat_t *tf)
{
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_traverse_dir(thinfat_t *tf, thinfat_core_event_t event)
{
  thinfat_sector_t sc_read;
  if (tf->cur_dir->ci_head == tf->ci_root)
    sc_read = thinfat_root_sector_count(tf);
  else
    sc_read = 0xFFFFFFFF;
  thinfat_blk_rewind(tf->cur_dir);
  return thinfat_blk_read_each_sector(tf, tf->cur_dir, sc_read, event);
}

thinfat_result_t thinfat_dump_current_directory(thinfat_t *tf, thinfat_event_t event)
{
  tf->event = event;
  return thinfat_traverse_dir(tf, THINFAT_CORE_EVENT_DUMP_DIR);
}

thinfat_result_t thinfat_find_file(thinfat_t *tf, const char *name, thinfat_event_t event)
{
  tf->event = event;
  tf->target_name = (const void *)name;
  return thinfat_traverse_dir(tf, THINFAT_CORE_EVENT_FIND_FILE);
}

thinfat_result_t thinfat_find_file_by_longname(thinfat_t *tf, const wchar_t *longname, thinfat_event_t event)
{
  tf->event = event;
  tf->target_name = (const void *)longname;
  tf->nc_matched = 0;
  return thinfat_traverse_dir(tf, THINFAT_CORE_EVENT_FIND_FILE_BY_LONGNAME);
}

thinfat_result_t thinfat_chdir(thinfat_t *tf, thinfat_cluster_t ci)
{
  return thinfat_blk_open(tf->cur_dir, ci);
}

thinfat_result_t thinfat_open_file(thinfat_t *tf, thinfat_cluster_t ci)
{
  return thinfat_blk_open(tf->cur_file, ci);
}

thinfat_result_t thinfat_read_file(thinfat_t *tf, void *buf, size_t size, thinfat_event_t event)
{
  thinfat_sector_t sc_read = ((tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE) + size + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
  tf->cur_file->byte_advance = size;
  tf->cur_file->user_buffer = buf;
  tf->event = event;
  return thinfat_blk_read_each_cluster(tf, tf->cur_file, sc_read, THINFAT_CORE_EVENT_READ_FILE);
}

thinfat_result_t thinfat_write_file(thinfat_t *tf, const void *buf, size_t size, thinfat_event_t event)
{
  thinfat_sector_t sc_write = ((tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE) + size + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
  tf->cur_file->byte_advance = size;
  tf->cur_file->user_buffer = (void *)buf;
  tf->event = event;
  return thinfat_blk_write_each_cluster(tf, tf->cur_file, sc_write, THINFAT_CORE_EVENT_WRITE_FILE);
}

thinfat_result_t thinfat_allocate_cluster(thinfat_t *tf, thinfat_cluster_t cc_allocate, thinfat_event_t event)
{
  tf->event = event;
}
