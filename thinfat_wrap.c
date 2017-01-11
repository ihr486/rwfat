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
  thinfat_result_t res;
  thinfat_phy_lock(tf->phy);
  tf->phy->cb_flag = false;
  if ((res = thinfat_find_partition(tf, THINFAT_USER_EVENT)) != THINFAT_RESULT_OK)
    return res;
  while (!tf->phy->cb_flag)
    thinfat_phy_wait(tf->phy);
  thinfat_phy_unlock(tf->phy);
  return THINFAT_RESULT_OK;
}

thinfat_result_t tfwrap_mount(thinfat_t *tf, thinfat_sector_t si_base)
{
  thinfat_result_t res;
  thinfat_phy_lock(tf->phy);
  tf->phy->cb_flag = false;
  if ((res = thinfat_mount(tf, si_base, THINFAT_USER_EVENT)) != THINFAT_RESULT_OK)
    return res;
  while (!tf->phy->cb_flag)
    thinfat_phy_wait(tf->phy);
  thinfat_phy_unlock(tf->phy);
  printf("Partition at 0x%08X mounted.\n", tf->phy->s_param_cb);
  return THINFAT_RESULT_OK;
}

thinfat_result_t tfwrap_unmount(thinfat_t *tf)
{
  thinfat_result_t res;
  thinfat_phy_lock(tf->phy);
  tf->phy->cb_flag = false;
  if ((res = thinfat_unmount(tf, THINFAT_USER_EVENT)) != THINFAT_RESULT_OK)
    return res;
  while (!tf->phy->cb_flag)
    thinfat_phy_wait(tf->phy);
  thinfat_phy_unlock(tf->phy);
  return THINFAT_RESULT_OK;
}
