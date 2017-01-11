/*!
 * @file thinfat_phy.h
 * @brief thinFAT common PHY layer interface
 * @date 2017/01/07
 * @author Hiroka IHARA
 */
#ifndef THINFAT_PHY_H
#define THINFAT_PHY_H

#include "thinfat_common.h"

#include <stdbool.h>
#include <pthread.h>

typedef enum
{
  THINFAT_PHY_STATE_IDLE = 0,
  THINFAT_PHY_STATE_SINGLE_READ,
  THINFAT_PHY_STATE_SINGLE_WRITE,
  THINFAT_PHY_STATE_MULTIPLE_READ,
  THINFAT_PHY_STATE_MULTIPLE_WRITE
}
thinfat_phy_state_t;

typedef struct thinfat_phy_tag
{
  thinfat_phy_state_t state;
  int fd;
  void *mapped_block;
  thinfat_sector_t si_mapped, sc_mapped;
  thinfat_sector_t si_req, sc_req;
  thinfat_sector_t sc_current;
  void *block;
  void *client;
  thinfat_core_event_t event;
  pthread_t thread;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  bool exit_flag;
  thinfat_sector_t s_param_cb;
  void *p_param_cb;
  bool cb_flag;
}
thinfat_phy_t;

typedef struct thinfat_time_tag
{
  union
  {
    uint16_t time;
    struct
    {
      uint16_t second2 : 5;
      uint16_t minute : 6;
      uint16_t hour : 5;
    };
  };
  union
  {
    uint16_t date;
    struct
    {
      uint16_t day : 5;
      uint16_t month : 4;
      uint16_t year : 7;
    };
  };
}
thinfat_time_t;

thinfat_result_t thinfat_core_callback(void *client, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, const char *devpath);
thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_finalize(thinfat_phy_t *phy);
bool thinfat_phy_is_idle(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_read_single(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_core_event_t event);
thinfat_result_t thinfat_phy_write_single(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_core_event_t event);
thinfat_result_t thinfat_phy_read_multiple(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_core_event_t event);
thinfat_result_t thinfat_phy_write_multiple(void *client, thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_core_event_t event);
thinfat_result_t thinfat_phy_get_time(thinfat_phy_t *phy, thinfat_time_t *data);

thinfat_result_t thinfat_phy_start(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_stop(thinfat_phy_t *phy);
void thinfat_phy_lock(thinfat_phy_t *phy);
void thinfat_phy_unlock(thinfat_phy_t *phy);
void thinfat_phy_wait(thinfat_phy_t *phy);
void thinfat_phy_signal(thinfat_phy_t *phy);
void thinfat_phy_enter(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_leave(thinfat_phy_t *phy, thinfat_result_t res);

#endif
