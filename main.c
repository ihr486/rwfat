/*!
 * @file main.c
 * @brief Test program for RivieraWaves FAT driver running on POSIX
 * @date 2017/01/07
 * @author Hiroka IHARA
 */

#include <stdio.h>
#include <stdlib.h>
#include "thinfat.h"
#include "thinfat_phy.h"

thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_EVENT_FIND_PARTITION:
    printf("Mounting partition at 0x%08X\n", s_param);
    thinfat_mount(tf, s_param);
    break;
  }
  return THINFAT_RESULT_OK;
}

int main(int argc, const char *argv[])
{
  printf("RivieraWaves FAT16/FAT32 driver testbed v0.1a\n");

  if (argc < 2)
  {
    fprintf(stderr, "Please specify the block device.\n");
    return EXIT_FAILURE;
  }

  thinfat_phy_t phy;
  thinfat_t tf;
  if (thinfat_phy_initialize(&phy, &tf, argv[1]) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to initialize the PHY.\n");
    return EXIT_FAILURE;
  }

  if (thinfat_initialize(&tf, &phy) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to initialize thinFAT.\n");
    return EXIT_FAILURE;
  }

  //thinfat_find_partition(&tf);
  thinfat_mount(&tf, 0);

  while(!thinfat_phy_is_idle(&phy))
  {
    if (thinfat_phy_schedule(&phy) != THINFAT_RESULT_OK)
      break;
  }

  thinfat_finalize(&tf);

  thinfat_phy_finalize(&phy);

  return EXIT_SUCCESS;
}
