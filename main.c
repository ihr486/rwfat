/*
 * Test program for RivieraWaves FAT driver
 * Copyright 2016 Hiroka IHARA <ihara_h@hongotechgarage.jp>
 */

#include <stdio.h>
#include <stdlib.h>
#include "thinfat.h"
#include "thinfat_phy.h"

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

  thinfat_finalize(&tf);

  return EXIT_SUCCESS;
}
