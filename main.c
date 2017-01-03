/*
 * Test program for RivieraWaves FAT driver
 * Copyright 2016 Hiroka IHARA <ihara_h@hongotechgarage.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "thinfat.h"
#include "thinfat_phy.h"

static thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_param_t param1, thinfat_param_t param2)
{
  switch(event)
  {
  case THINFAT_EVENT_FIND_PARTITION:
    printf("Mounting partition at 0x%08X\n", param1);
    thinfat_mount(tf, (thinfat_sector_t)param1);
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

  if (thinfat_initialize(&tf, &phy, thinfat_user_callback) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to initialize thinFAT.\n");
    return EXIT_FAILURE;
  }

  //thinfat_find_partition(&tf);
  thinfat_mount(&tf, 0);

  while(!thinfat_phy_is_idle(&phy))
  {
    thinfat_phy_schedule(&phy);
  }

  thinfat_finalize(&tf);

  thinfat_phy_finalize(&phy);

  return EXIT_SUCCESS;
}
