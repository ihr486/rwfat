#include "thinfat_phy.h"

#include <stdio.h>

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, void *instance, const char *devpath)
{
  phy->state = THINFAT_PHY_STATE_IDLE;
  phy->fp = fopen(devpath, "w+b");
  phy->instance = instance;
  return phy->fp == NULL ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy)
{
  switch(phy->state)
  {
  case THINFAT_PHY_STATE_IDLE:
    break;
  case THINFAT_PHY_STATE_SINGLE_READ:
    if (fseek(phy->fp, THINFAT_SECTOR_SIZE * phy->sector, SEEK_SET))
      return THINFAT_RESULT_PHY_ERROR;
    if (fread(phy->block, THINFAT_SECTOR_SIZE, 1, phy->fp) != THINFAT_SECTOR_SIZE)
      return THINFAT_RESULT_PHY_ERROR;
    if (phy->callback != NULL)
      phy->callback(phy->instance, phy->block);
    phy->state = THINFAT_PHY_STATE_IDLE;
    break;
  case THINFAT_PHY_STATE_SINGLE_WRITE:
    if (fseek(phy->fp, THINFAT_SECTOR_SIZE * phy->sector, SEEK_SET))
      return THINFAT_RESULT_PHY_ERROR;
    if (fread(phy->block, THINFAT_SECTOR_SIZE, 1, phy->fp) != THINFAT_SECTOR_SIZE)
      return THINFAT_RESULT_PHY_ERROR;
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
  return fclose(phy->fp) ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
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

thinfat_result_t thinfat_phy_set_instance(thinfat_phy_t *phy, void *instance)
{
  phy->instance = instance;
  return THINFAT_RESULT_OK;
}
