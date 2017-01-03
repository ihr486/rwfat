/*
 * POSIX PHY layer for RivieraWaves FAT driver
 * Copyright 2017 Hiroka IHARA <ihara_h@hongotechgarage.jp>
 */
#include "thinfat_phy.h"

#include <stdio.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, void *instance, const char *devpath)
{
  phy->state = THINFAT_PHY_STATE_IDLE;
  phy->fd = open(devpath, O_RDWR);
  phy->instance = instance;
  phy->mapped_block = NULL;
  return phy->fd == NULL ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
}

bool thinfat_phy_is_idle(thinfat_phy_t *phy)
{
  return phy->state == THINFAT_PHY_STATE_IDLE;
}

thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy)
{
  long sc_pagesize = sysconf(_SC_PAGESIZE) / THINFAT_SECTOR_SIZE;
  switch(phy->state)
  {
  case THINFAT_PHY_STATE_IDLE:
    break;
  case THINFAT_PHY_STATE_SINGLE_READ:
    if (phy->sector < phy->mapped_sector || phy->mapped_sector + phy->sc_mapped <= phy->sector)
    {
      if (phy->mapped_block != NULL)
        munmap(phy->mapped_block, THINFAT_SECTOR_SIZE * phy->sc_mapped);
      phy->mapped_block = mmap(NULL, THINFAT_SECTOR_SIZE * sc_pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, phy->fd, THINFAT_SECTOR_SIZE * (phy->sector / sc_pagesize * sc_pagesize));
      phy->sc_mapped = sc_pagesize;
      phy->mapped_sector = phy->sector / sc_pagesize * sc_pagesize;
    }
    {
      void *src = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * (phy->sector % sc_pagesize);
      memcpy(phy->block, src, THINFAT_SECTOR_SIZE);
    }
    if (phy->callback != NULL)
      phy->callback(phy->instance, phy->block);
    phy->state = THINFAT_PHY_STATE_IDLE;
    break;
  case THINFAT_PHY_STATE_SINGLE_WRITE:
    if (phy->sector < phy->mapped_sector || phy->mapped_sector + phy->sc_mapped <= phy->sector)
    {
      if (phy->mapped_block != NULL)
        munmap(phy->mapped_block, THINFAT_SECTOR_SIZE * phy->sc_mapped);
      phy->mapped_block = mmap(NULL, THINFAT_SECTOR_SIZE * sc_pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, phy->fd, THINFAT_SECTOR_SIZE * phy->sector);
      phy->mapped_sector = phy->sector / sc_pagesize * sc_pagesize;
      phy->sc_mapped = sc_pagesize;
    }
    {
      void *dest = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * (phy->sector % sc_pagesize);
      memcpy(dest, phy->block, THINFAT_SECTOR_SIZE);
    }
    if (phy->callback != NULL)
      phy->callback(phy->instance, phy->block);
    phy->state = THINFAT_PHY_STATE_IDLE;
    break;
  case THINFAT_PHY_STATE_MULTIPLE_READ:
    break;
  case THINFAT_PHY_STATE_MULTIPLE_WRITE:
    break;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_finalize(thinfat_phy_t *phy)
{
  if (phy->mapped_block != NULL)
    munmap(phy->mapped_block, THINFAT_SECTOR_SIZE * phy->sc_mapped);
  return close(phy->fd) ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_read_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_callback_t callback)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->state = THINFAT_PHY_STATE_SINGLE_READ;
  phy->sector = sector;
  phy->count = 1;
  phy->block = block;
  phy->callback = callback;
  THINFAT_INFO("Single READ request @ 0x%08X\n", sector);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_write_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_callback_t callback)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->state = THINFAT_PHY_STATE_SINGLE_WRITE;
  phy->sector = sector;
  phy->count = 1;
  phy->block = block;
  phy->callback = callback;
  THINFAT_INFO("Single WRITE request @ 0x%08X\n", sector);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_read_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_callback_t callback)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->state = THINFAT_PHY_STATE_MULTIPLE_READ;
  phy->sector = sector;
  phy->count = count;
  phy->callback = callback;
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_write_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_callback_t callback)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->state = THINFAT_PHY_STATE_MULTIPLE_WRITE;
  phy->sector = sector;
  phy->count = count;
  phy->callback = callback;
  return THINFAT_RESULT_OK;
}
