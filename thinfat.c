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
#include "thinfat_table.h"
#include "thinfat_file.h"
#include "thinfat_dir.h"

#include <stdlib.h>
#include <string.h>

static thinfat_result_t thinfat_read_mbr_callback(thinfat_t *tf, void *mbr);
static thinfat_result_t thinfat_read_parameter_block_callback(thinfat_t *tf, void *bpb);
static thinfat_result_t thinfat_read_fsinfo_callback(thinfat_t *tf, void *fsi);

/*static void thinfat_memcpy(void *dest, const void *src, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
  }
}*/

thinfat_result_t thinfat_core_callback(void *instance, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  THINFAT_INFO("Core callback: %p, %d, " TFF_X32 ", %p\n", instance, event, s_param, p_param);
  if (event < THINFAT_CORE_EVENT_MAX)
  {
    switch(event)
    {
    case THINFAT_CORE_EVENT_READ_MBR:
      return thinfat_read_mbr_callback((thinfat_t *)instance, *(void **)p_param);
    case THINFAT_CORE_EVENT_READ_BPB:
      return thinfat_read_parameter_block_callback((thinfat_t *)instance, *(void **)p_param);
    case THINFAT_CORE_EVENT_READ_FSINFO:
      return thinfat_read_fsinfo_callback((thinfat_t *)instance, *(void **)p_param);
    }
  }
  else if (event < THINFAT_CACHE_EVENT_MAX)
    return thinfat_cache_callback((thinfat_cache_t *)instance, event, s_param, p_param);
  else if (event < THINFAT_BLK_EVENT_MAX)
    return thinfat_blk_callback((thinfat_blk_t *)instance, event, s_param, p_param);
  else if (event < THINFAT_TABLE_EVENT_MAX)
    return thinfat_table_callback((thinfat_table_t *)instance, event, s_param, p_param);
  else if (event < THINFAT_FILE_EVENT_MAX)
    return thinfat_file_callback((thinfat_file_t *)instance, event, s_param, p_param);
  else if (event < THINFAT_DIR_EVENT_MAX)
    return thinfat_dir_callback((thinfat_dir_t *)instance, event, s_param, p_param);

  return thinfat_user_callback((thinfat_t *)instance, event, s_param, p_param);
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

  thinfat_dir_open(tf->cur_dir, tf->ci_root);

  if (tf->type == THINFAT_TYPE_FAT16)
  {
    tf->ci_next_free = 2;
    tf->cc_free = 0;

    return thinfat_core_callback(tf, tf->event, tf->si_hidden, NULL);
  }
  else
  {
    thinfat_sector_t si_fsinfo = tf->si_hidden + thinfat_read_u16(bpb, 48);

    return thinfat_cached_read_single(tf, tf->table_cache, si_fsinfo, THINFAT_CORE_EVENT_READ_FSINFO);
  }
}

static thinfat_result_t thinfat_read_fsinfo_callback(thinfat_t *tf, void *fsi)
{
  uint32_t leadsig = thinfat_read_u32(fsi, 0);
  if (leadsig != 0x41615252)
    return THINFAT_RESULT_ERROR_SIGNATURE;

  uint32_t strucsig = thinfat_read_u32(fsi, 484);
  if (strucsig != 0x61417272)
    return THINFAT_RESULT_ERROR_SIGNATURE;

  uint32_t trailsig = thinfat_read_u32(fsi, 508);
  if (trailsig != 0xAA550000)
    return THINFAT_RESULT_ERROR_SIGNATURE;

  tf->cc_free = thinfat_read_u32(fsi, 488);
  tf->ci_next_free = thinfat_read_u32(fsi, 492);
    
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

thinfat_result_t thinfat_find_partition(thinfat_t *tf, thinfat_event_t event)
{
  tf->event = event;
  return thinfat_cached_read_single(tf, tf->table_cache, 0, THINFAT_CORE_EVENT_READ_MBR);
}

thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_sector_t sector, thinfat_event_t event)
{
  tf->event = event;
  tf->si_hidden = sector;
  return thinfat_cached_read_single(tf, tf->table_cache, sector, THINFAT_CORE_EVENT_READ_BPB);
}

thinfat_result_t thinfat_unmount(thinfat_t *tf, thinfat_event_t event)
{
  tf->event = event;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy)
{
  tf->phy = phy;

  tf->table_cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(tf->table_cache, tf);
  tf->dir_cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(tf->dir_cache, tf);
  tf->file_cache = (thinfat_cache_t *)malloc(sizeof(thinfat_cache_t));
  thinfat_cache_init(tf->file_cache, tf);

  tf->table = (thinfat_table_t *)malloc(sizeof(thinfat_table_t));
  thinfat_table_init(tf->table, tf, tf->table_cache);

  tf->cur_dir = (thinfat_dir_t *)malloc(sizeof(thinfat_dir_t));
  thinfat_dir_init(tf->cur_dir, tf, tf->dir_cache);

  tf->cur_file = (thinfat_file_t *)malloc(sizeof(thinfat_file_t));
  thinfat_file_init(tf->cur_file, tf, tf->file_cache);

  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_finalize(thinfat_t *tf)
{
  free(tf->table);
  free(tf->cur_dir);
  free(tf->cur_file);

  free(tf->table_cache);
  free(tf->dir_cache);
  free(tf->file_cache);

  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_dump_current_directory(thinfat_t *tf, thinfat_event_t event)
{
  return thinfat_dir_dump(tf, tf->cur_dir, event);
}

thinfat_result_t thinfat_find_file_by_longname(thinfat_t *tf, const wchar_t *name, thinfat_event_t event)
{
  return thinfat_dir_find_by_longname(tf, tf->cur_dir, name, event);
}

thinfat_result_t thinfat_open_file(thinfat_t *tf, const thinfat_dir_entry_t *entry)
{
  return thinfat_file_open(tf->cur_file, entry);
}

thinfat_result_t thinfat_read_file(thinfat_t *tf, void *buf, size_t size, thinfat_event_t event)
{
  return thinfat_file_read(tf, tf->cur_file, buf, size, event);
}

thinfat_result_t thinfat_write_file(thinfat_t *tf, const void *buf, size_t size, thinfat_event_t event)
{
  return thinfat_file_write(tf, tf->cur_file, buf, size, event);
}
