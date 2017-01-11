#include "thinfat_wrap.h"

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_EVENT_FIND_PARTITION:
    printf("Mounting partition at 0x%08X\n", s_param);
    thinfat_mount(tf, s_param, THINFAT_EVENT_MOUNT);
    break;
  case THINFAT_EVENT_MOUNT:
    printf("Partition at 0x%08X mounted.\n", s_param);
    //thinfat_dump_current_directory(tf, THINFAT_USER_EVENT + 1);
    thinfat_find_file_by_longname(tf, L"U3D_A_061228_5", THINFAT_USER_EVENT + 1);
    break;
  case THINFAT_USER_EVENT + 1:
    printf("User event #1 detected.\n");
    printf("Cluster = " TFF_X32 "\n", ((thinfat_dir_entry_t *)p_param)->ci_head);
    break;
  case THINFAT_EVENT_UNMOUNT:
    break;
  }
  return THINFAT_RESULT_OK;
}

thinfat_result_t tfwrap_find_partition(thinfat_t *tf)
{
  thinfat_phy_lock(tf->phy);
  thinfat_phy_unlock(tf->phy);
}

thinfat_result_t tfwrap_mount(thinfat_t *tf)
{
  thinfat_phy_lock(tf->phy);
  thinfat_phy_unlock(tf->phy);
}

thinfat_result_t tfwrap_unmount(thinfat_t *tf)
{
  thinfat_phy_lock(tf->phy);
  thinfat_phy_unlock(tf->phy);
}
