#ifndef THINFAT_PHY_H
#define THINFAT_PHY_H

#include "thinfat_common.h"

typedef enum
{
  THINFAT_PHY_STATE_IDLE = 0,
  THINFAT_PHY_STATE_SINGLE_READ,
  THINFAT_PHY_STATE_SINGLE_WRITE,
  THINFAT_PHY_STATE_MULTIPLE_READ,
  THINFAT_PHY_STATE_MULTIPLE_WRITE
}
thinfat_phy_state_t;

typedef thinfat_result_t (*thinfat_phy_callback_t)(void *instance, void *data);

typedef struct thinfat_phy_tag
{
  thinfat_phy_state_t state;
  FILE *fp;
  thinfat_sector_t sector;
  thinfat_sector_t count;
  void *block;
  void *instance;
  thinfat_phy_callback_t callback;
}
thinfat_phy_t;

thinfat_result_t thinfat_phy_initialize(thinfat_phy_t *phy, void *instance, const char *devpath);
thinfat_result_t thinfat_phy_schedule(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_finalize(thinfat_phy_t *phy);
thinfat_result_t thinfat_phy_read_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_callback_t callback);
thinfat_result_t thinfat_phy_write_single(thinfat_phy_t *phy, thinfat_sector_t sector, void *block, thinfat_phy_callback_t callback);
thinfat_result_t thinfat_phy_read_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_callback_t callback);
thinfat_result_t thinfat_phy_write_multiple(thinfat_phy_t *phy, thinfat_sector_t sector, thinfat_sector_t count, thinfat_phy_callback_t callback);

#endif
