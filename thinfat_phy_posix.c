/*!
 * @file thinfat_phy_posix.c
 * @brief POSIX PHY layer for RivieraWaves FAT driver <br>
 *        This is meant to be an example PHY implementation.
 * @date 2017/01/07
 * @author Hiroka IHARA
 */
#include "thinfat_phy.h"

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>

static thinfat_sector_t sc_pagesize = 0;

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, const char *devpath)
{
  phy->state = THINFAT_PHY_STATE_IDLE;
  phy->fd = open(devpath, O_RDWR);
  phy->mapped_block = NULL;
  sc_pagesize = sysconf(_SC_PAGESIZE) / THINFAT_SECTOR_SIZE;
  return phy->fd < 0 ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
}

bool thinfat_phy_is_idle(thinfat_phy_t *phy)
{
  return phy->state == THINFAT_PHY_STATE_IDLE;
}

static thinfat_result_t thinfat_phy_remap(thinfat_phy_t *phy)
{
  if (phy->mapped_block == NULL || phy->si_req < phy->si_mapped || phy->si_mapped + phy->sc_mapped <= phy->si_req)
  {
    if (phy->mapped_block != NULL)
      munmap(phy->mapped_block, THINFAT_SECTOR_SIZE * phy->sc_mapped);
    phy->si_mapped = phy->si_req / sc_pagesize * sc_pagesize;
    phy->mapped_block = mmap(NULL, THINFAT_SECTOR_SIZE * sc_pagesize, PROT_READ/* | PROT_WRITE*/, MAP_SHARED, phy->fd, THINFAT_SECTOR_SIZE * phy->si_mapped);
    phy->sc_mapped = sc_pagesize;
    if (phy->mapped_block == (void *)-1)
      return THINFAT_RESULT_PHY_ERROR;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy)
{
  thinfat_result_t res;

  switch(phy->state)
  {
  case THINFAT_PHY_STATE_IDLE:
    break;
  case THINFAT_PHY_STATE_SINGLE_READ:
    if ((res = thinfat_phy_remap(phy)) == THINFAT_RESULT_OK)
    {
      void *src = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * (phy->si_req % sc_pagesize);
      memcpy(phy->block, src, THINFAT_SECTOR_SIZE);
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, 0, &phy->block);
    }
    else
    {
      return res;
    }
    break;
  case THINFAT_PHY_STATE_SINGLE_WRITE:
    if ((res = thinfat_phy_remap(phy)) == THINFAT_RESULT_OK)
    {
      void *dest = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * (phy->si_req % sc_pagesize);
      memcpy(dest, phy->block, THINFAT_SECTOR_SIZE);
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, 0, &phy->block);
    }
    else
    {
      return res;
    }
    break;
  case THINFAT_PHY_STATE_MULTIPLE_READ:
    if (phy->block == NULL)
    {
      if ((res = thinfat_phy_remap(phy)) == THINFAT_RESULT_OK)
      {
        return thinfat_core_callback(phy->client, phy->event, 0, &phy->block);
      }
      else
      {
        return res;
      }
    }
    else if (phy->sc_current < phy->sc_req)
    {
      void *src = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * ((phy->si_req % sc_pagesize) + phy->sc_current);
      memcpy(phy->block, src, THINFAT_SECTOR_SIZE);
      return thinfat_core_callback(phy->client, phy->event, phy->sc_current++, &phy->block);
    }
    else
    {
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, phy->sc_current, NULL);
    }
    break;
  case THINFAT_PHY_STATE_MULTIPLE_WRITE:
    if (phy->block == NULL)
    {
      if ((res = thinfat_phy_remap(phy)) == THINFAT_RESULT_OK)
      {
        return thinfat_core_callback(phy->client, phy->event, 0, &phy->block);
      }
      else
      {
        return res;
      }
    }
    else if (phy->sc_current < phy->sc_req)
    {
      void *dest = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * ((phy->si_req % sc_pagesize) + phy->sc_current);
      memcpy(dest, phy->block, THINFAT_SECTOR_SIZE);
      return thinfat_core_callback(phy->client, phy->event, phy->sc_current++, &phy->block);
    }
    else
    {
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, phy->sc_current, NULL);
    }
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

thinfat_result_t thinfat_phy_read_single(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_core_event_t event)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->client = client;
  phy->state = THINFAT_PHY_STATE_SINGLE_READ;
  phy->si_req = sector;
  phy->sc_req = 1;
  phy->sc_current = 0;
  phy->block = block;
  phy->event = event;
  THINFAT_INFO("Single READ request @ " TFF_X32 "\n", sector);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_write_single(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_core_event_t event)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->client = client;
  phy->state = THINFAT_PHY_STATE_SINGLE_WRITE;
  phy->si_req = sector;
  phy->sc_req = 1;
  phy->sc_current = 0;
  phy->block = block;
  phy->event = event;
  THINFAT_INFO("Single WRITE request @ " TFF_X32 "\n", sector);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_read_multiple(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_core_event_t event)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->client = client;
  phy->state = THINFAT_PHY_STATE_MULTIPLE_READ;
  phy->si_req = sector;
  phy->sc_req = count;
  phy->sc_current = 0;
  phy->block = NULL;
  phy->event = event;
  THINFAT_INFO("Multiple READ request @ " TFF_X32 " * " TFF_U32 "\n", sector, count);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_write_multiple(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_core_event_t event)
{
  if (phy->state != THINFAT_PHY_STATE_IDLE)
  {
    return THINFAT_RESULT_PHY_BUSY;
  }
  phy->client = client;
  phy->state = THINFAT_PHY_STATE_MULTIPLE_WRITE;
  phy->si_req = sector;
  phy->sc_req = count;
  phy->sc_current = 0;
  phy->block = NULL;
  phy->event = event;
  THINFAT_INFO("Multiple WRITE request @ " TFF_X32 " * " TFF_U32 "\n", sector, count);
  return THINFAT_RESULT_OK;
}
