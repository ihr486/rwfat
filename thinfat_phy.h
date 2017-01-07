/*!
 * @file thinfat_phy.h
 * @brief Common PHY layer interface for RivieraWaves FAT driver
 * @date 2017/01/07
 * @author Hiroka IHARA
 */
#ifndef THINFAT_PHY_H
#define THINFAT_PHY_H

#include "thinfat_common.h"

#include <stdbool.h>

typedef enum
{
  THINFAT_PHY_STATE_IDLE = 0,
  THINFAT_PHY_STATE_SINGLE_READ,
  THINFAT_PHY_STATE_SINGLE_WRITE,
  THINFAT_PHY_STATE_MULTIPLE_READ,
  THINFAT_PHY_STATE_MULTIPLE_WRITE
}
thinfat_phy_state_t;

typedef enum
{
  THINFAT_PHY_EVENT_READ_MBR = 1,
  THINFAT_PHY_EVENT_READ_BPB
}
thinfat_phy_event_t;

typedef struct thinfat_phy_tag
{
  thinfat_phy_state_t state;
  int fd;
  void *mapped_block;
  thinfat_sector_t si_mapped, sc_mapped;
  thinfat_sector_t si_req, sc_req;
  thinfat_sector_t sc_current;
  void *block;
  void *instance;
  thinfat_phy_event_t event;
}
thinfat_phy_t;

thinfat_result_t thinfat_phy_callback(void *instance, thinfat_phy_event_t event, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, void *instance, const char *devpath);
thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_finalize(thinfat_phy_t *phy);
bool thinfat_phy_is_idle(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_read_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_event_t event);
thinfat_result_t thinfat_phy_write_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_event_t event);
thinfat_result_t thinfat_phy_read_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_event_t event);
thinfat_result_t thinfat_phy_write_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_event_t event);

#endif
