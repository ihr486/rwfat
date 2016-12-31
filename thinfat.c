#include "thinfat.h"

static thinfat_type_t thinfat_determine_type(const thinfat_t *tf)
{
    thinfat_cluster_t cluster_count = (tf->sc_volume_size - tf->si_data) >> tf->cluster_shift;
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

thinfat_result_t thinfat_read_parameter_block_callback(void *_tf, void *bpb)
{
  thinfat_t *tf = (thinfat_t *)_tf;
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
  for (tf->cluster_shift = 0; !(BPB_SecPerClus & 1); BPB_SecPerClus >>= 1)
  {
      tf->cluster_shift++;
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

  tf->ci_root = thinfat_read_u32(bpb, 44);
  THINFAT_INFO("Root cluster: %u\n", tf->ci_root);

  tf->si_data = tf->si_hidden + tf->sc_reserved + tf->sc_table_size * tf->table_redundancy + (tf->root_entry_count * 32 + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;

  tf->type = thinfat_determine_type(tf);

  if (tf->type == THINFAT_TYPE_FAT32)
  {
    tf->ci_current_dir = tf->ci_root;
  }
  else
  {
    tf->ci_current_dir = 0;
  }

  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_read_parameter_block(thinfat_t *tf)
{
  thinfat_result_t res;
  res = thinfat_phy_read_single(tf->phy, tf->si_hidden, THINFAT_TABLE_CACHE(tf), thinfat_read_parameter_block_callback);
  return res;
}

static thinfat_result_t thinfat_read_mbr_callback(void *_tf, void *mbr)
{
  thinfat_t *tf = (thinfat_t *)_tf;
  uint16_t signature = thinfat_read_u16(mbr, 510);
  if (signature != 0xAA55)
  {
    THINFAT_ERROR("Corrupt MBR signature detected.\n");
    return THINFAT_RESULT_ERROR_SIGNATURE;
  }

  for (unsigned int i = 0; i < 4; i++)
  {
    uint8_t partition_type = thinfat_read_u8(mbr, 446 + i * 16 + 4);

    switch (partition_type)
    {
    case 0x0B:
    case 0x0C:
      tf->si_hidden = thinfat_read_u32(mbr, 446 + i * 16 + 8);
      THINFAT_INFO("Partition %u@%u: FAT32(LBA)\n", i, tf->si_hidden);
      return thinfat_read_parameter_block(tf);
    case 0x04:
    case 0x06:
    case 0x0E:
      tf->si_hidden = thinfat_read_u32(mbr, 446 + i * 16 + 8);
      THINFAT_INFO("Partition %u@%u: FAT16(LBA)\n", i, tf->si_hidden);
      return thinfat_read_parameter_block(tf);
    }
  }
  THINFAT_ERROR("No partition found on the disk.\n");
  return THINFAT_RESULT_NO_PARTITION;
}

thinfat_result_t thinfat_mount(thinfat_t *tf, thinfat_user_callback_t callback)
{
  thinfat_result_t res;
  res = thinfat_phy_read_single(tf->phy, 0, THINFAT_TABLE_CACHE(tf), thinfat_read_mbr_callback);
  if (res != THINFAT_RESULT_OK)
    return res;
  tf->callback = callback;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_unmount(thinfat_t *tf)
{
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_initialize(thinfat_t *tf, struct thinfat_phy_tag *phy)
{
  tf->phy = phy;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_finalize(thinfat_t *tf)
{
  return THINFAT_RESULT_OK;
}
