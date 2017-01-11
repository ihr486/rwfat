/*!
 * @file thinfat_wrap.c
 * @brief thinFAT synchronous wrapper
 * @date 2017/01/12
 * @author Hiroka IHARA
 */
#include "thinfat_wrap.h"
#include "thinfat_phy.h"

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_USER_EVENT:
    printf("User event #1 detected.\n");
    tf->phy->s_param_cb = s_param;
    tf->phy->p_param_cb = p_param;
    tf->phy->cb_flag = true;
    thinfat_phy_signal(tf->phy);
    break;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t tfwrap_find_partition(thinfat_t *tf)
{
  thinfat_phy_enter(tf->phy);
  return thinfat_phy_leave(tf->phy, thinfat_find_partition(tf, THINFAT_USER_EVENT));
}

thinfat_result_t tfwrap_mount(thinfat_t *tf, thinfat_sector_t si_base)
{
  thinfat_phy_enter(tf->phy);
  return thinfat_phy_leave(tf->phy, thinfat_mount(tf, si_base, THINFAT_USER_EVENT));
}

thinfat_result_t tfwrap_unmount(thinfat_t *tf)
{
  thinfat_phy_enter(tf->phy);
  return thinfat_phy_leave(tf->phy, thinfat_unmount(tf, THINFAT_USER_EVENT));
}

thinfat_result_t tfwrap_dump_current_directory(thinfat_t *tf)
{
  thinfat_phy_enter(tf->phy);
  return thinfat_phy_leave(tf->phy, thinfat_dump_current_directory(tf, THINFAT_USER_EVENT));
}
