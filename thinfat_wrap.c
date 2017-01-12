/*!
 * @file thinfat_wrap.c
 * @brief thinFAT synchronous wrapper
 * @date 2017/01/12
 * @author Hiroka IHARA
 */
#include "thinfat_wrap.h"
#include "thinfat_phy.h"

#include <string.h>

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_USER_EVENT:
    printf("User event #1 detected.\n");
    tf->phy->s_param_cb = s_param;
    tf->phy->p_param_cb = p_param;
    break;
  case THINFAT_EVENT_FIND_FILE:
    printf("Find file event detected.\n");
    if (p_param != NULL)
      memcpy(tf->phy->arg, p_param, sizeof(thinfat_dir_entry_t));
    else
      ((thinfat_dir_entry_t *)tf->phy->arg)->name[0] = 0x00;
    break;
  case THINFAT_EVENT_READ_FILE:
    break;
  case THINFAT_EVENT_WRITE_FILE:
    break;
  }
  tf->phy->cb_flag = true;
  thinfat_phy_signal(tf->phy);
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

thinfat_result_t tfwrap_find_file(thinfat_t *tf, const char *name, thinfat_dir_entry_t *entry)
{
  thinfat_phy_enter(tf->phy);
  tf->phy->arg = entry;
  return thinfat_phy_leave(tf->phy, thinfat_find_file(tf, name, THINFAT_EVENT_FIND_FILE));
}

thinfat_result_t tfwrap_find_file_by_longname(thinfat_t *tf, const wchar_t *longname, thinfat_dir_entry_t *entry)
{
  thinfat_phy_enter(tf->phy);
  tf->phy->arg = entry;
  return thinfat_phy_leave(tf->phy, thinfat_find_file_by_longname(tf, longname, THINFAT_EVENT_FIND_FILE));
}

thinfat_result_t tfwrap_read_file(thinfat_t *tf, void *buf, size_t size)
{
  thinfat_phy_enter(tf->phy);
  tf->phy->arg = buf;
  return thinfat_phy_leave(tf->phy, thinfat_read_file(tf, buf, size, THINFAT_EVENT_READ_FILE));
}

thinfat_result_t tfwrap_write_file(thinfat_t *tf, const void *buf, size_t size)
{
  thinfat_phy_enter(tf->phy);
  tf->phy->arg = buf;
  return thinfat_phy_leave(tf->phy, thinfat_write_file(tf, buf, size, THINFAT_EVENT_WRITE_FILE));
}
