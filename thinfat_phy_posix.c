/*!
 * @file thinfat_phy_posix.c
 * @brief POSIX PHY layer designed for thinFAT <br>
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
#include <time.h>

void thinfat_phy_lock(thinfat_phy_t *phy)
{
  pthread_mutex_lock(&phy->lock);
}

void thinfat_phy_unlock(thinfat_phy_t *phy)
{
  pthread_mutex_unlock(&phy->lock);
}

static void *thinfat_phy_execute(void *arg)
{
  thinfat_phy_t *phy = (thinfat_phy_t *)arg;
  while(!phy->exit_flag)
  {
    pthread_mutex_lock(&phy->lock);
    thinfat_phy_schedule(phy);
    pthread_mutex_unlock(&phy->lock);
    sleep(0);
  }
  return NULL;
}

void thinfat_phy_enter(thinfat_phy_t *phy)
{
  pthread_mutex_lock(&phy->lock);
  phy->cb_flag = false;
}

thinfat_result_t thinfat_phy_leave(thinfat_phy_t *phy, thinfat_result_t res)
{
  if (res != THINFAT_RESULT_OK) return res;
  while (!phy->cb_flag)
    pthread_cond_wait(&phy->cond, &phy->lock);
  pthread_mutex_unlock(&phy->lock);
  return THINFAT_RESULT_OK;
}

static thinfat_sector_t sc_pagesize = 0;

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, const char *devpath)
{
  phy->state = THINFAT_PHY_STATE_IDLE;
  phy->fd = open(devpath, O_RDWR);
  phy->mapped_block = NULL;
  sc_pagesize = sysconf(_SC_PAGESIZE) / THINFAT_SECTOR_SIZE;
  pthread_mutex_init(&phy->lock, NULL);
  pthread_cond_init(&phy->cond, NULL);
  return phy->fd < 0 ? THINFAT_RESULT_PHY_ERROR : THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_start(thinfat_phy_t *phy)
{
  phy->exit_flag = false;
  pthread_create(&phy->thread, NULL, thinfat_phy_execute, phy);
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_stop(thinfat_phy_t *phy)
{
  pthread_mutex_lock(&phy->lock);
  phy->exit_flag = true;
  pthread_mutex_unlock(&phy->lock);
  pthread_join(phy->thread, NULL);
  return THINFAT_RESULT_OK;
}

void thinfat_phy_wait(thinfat_phy_t *phy)
{
  pthread_cond_wait(&phy->cond, &phy->lock);
}

void thinfat_phy_signal(thinfat_phy_t *phy)
{
  pthread_cond_signal(&phy->cond);
}

bool thinfat_phy_is_idle(thinfat_phy_t *phy)
{
  return phy->state == THINFAT_PHY_STATE_IDLE;
}

static thinfat_result_t thinfat_phy_remap(thinfat_phy_t *phy)
{
  if (phy->mapped_block == NULL || phy->si_req < phy->si_mapped || phy->si_mapped + phy->sc_mapped < phy->si_req + phy->sc_req)
  {
    if (phy->mapped_block != NULL)
      munmap(phy->mapped_block, THINFAT_SECTOR_SIZE * phy->sc_mapped);
    phy->si_mapped = phy->si_req / sc_pagesize * sc_pagesize;
    phy->sc_mapped = ((phy->si_req % sc_pagesize) + phy->sc_req + sc_pagesize - 1) / sc_pagesize * sc_pagesize;
    phy->mapped_block = mmap(NULL, THINFAT_SECTOR_SIZE * phy->sc_mapped, PROT_READ/* | PROT_WRITE*/, MAP_SHARED, phy->fd, THINFAT_SECTOR_SIZE * phy->si_mapped);
    if (phy->mapped_block == (void *)-1)
      return THINFAT_RESULT_PHY_ERROR;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy)
{
  thinfat_result_t res;

  /*if (!thinfat_phy_is_idle(phy))
    THINFAT_INFO("PHY scheduler called: state = %d.\n", (int)phy->state);*/

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
      return thinfat_core_callback(phy->client, phy->event, phy->si_req, &phy->block);
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
      return thinfat_core_callback(phy->client, phy->event, phy->si_req, &phy->block);
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
        return thinfat_core_callback(phy->client, phy->event, phy->si_req, &phy->block);
      }
      else
      {
        return res;
      }
    }
    else if (phy->sc_current < phy->sc_req)
    {
      void *src = (uint8_t *)phy->mapped_block + THINFAT_SECTOR_SIZE * (phy->si_req - phy->si_mapped + phy->sc_current);
      memcpy(phy->block, src, THINFAT_SECTOR_SIZE);
      return thinfat_core_callback(phy->client, phy->event, phy->si_req + phy->sc_current++, &phy->block);
    }
    else
    {
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, phy->si_req + phy->sc_current, NULL);
    }
    break;
  case THINFAT_PHY_STATE_MULTIPLE_WRITE:
    if (phy->block == NULL)
    {
      if ((res = thinfat_phy_remap(phy)) == THINFAT_RESULT_OK)
      {
        return thinfat_core_callback(phy->client, phy->event, phy->si_req, &phy->block);
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
      phy->sc_current++;
      if (phy->sc_current < phy->sc_req)
        return thinfat_core_callback(phy->client, phy->event, phy->si_req + phy->sc_current - 1, &phy->block);
    }
    else
    {
      phy->state = THINFAT_PHY_STATE_IDLE;
      return thinfat_core_callback(phy->client, phy->event, phy->si_req + phy->sc_current, NULL);
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

thinfat_result_t thinfat_phy_get_time(thinfat_phy_t *phy, thinfat_time_t *data)
{
  (void)phy;
  time_t t = time(NULL);
  struct tm *ts = localtime(&t);
  data->year = ts->tm_year - 80;
  data->month = ts->tm_mon + 1;
  data->day = ts->tm_mday;
  data->hour = ts->tm_hour;
  data->minute = ts->tm_min;
  data->second2 = ts->tm_sec;
  return THINFAT_RESULT_OK;
}
