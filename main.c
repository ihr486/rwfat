/*!
 * @file main.c
 * @brief thinFAT test program running on POSIX environment
 * @date 2017/01/07
 * @author Hiroka IHARA
 */

#include <stdio.h>
#include <stdlib.h>
#include "thinfat.h"
#include "thinfat_phy.h"
#include "thinfat_blk.h"
#include "thinfat_cache.h"
#include "thinfat_wrap.h"

/*thinfat_result_t thinfat_user_callback(thinfat_t *tf, thinfat_event_t event, thinfat_sector_t s_param, void *p_param)
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
}*/

int main(int argc, const char *argv[])
{
  printf("thinFAT testbed v0.1a\n");

  if (argc < 2)
  {
    fprintf(stderr, "Please specify the block device.\n");
    return EXIT_FAILURE;
  }

  thinfat_phy_t phy;
  if (thinfat_phy_initialize(&phy, argv[1]) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to initialize the PHY.\n");
    return EXIT_FAILURE;
  }

  thinfat_t tf;
  if (thinfat_initialize(&tf, &phy) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to initialize thinFAT.\n");
    return EXIT_FAILURE;
  }

  thinfat_phy_start(&phy);

  tfwrap_mount(&tf, 0);

  tfwrap_dump_current_directory(&tf);

  thinfat_dir_entry_t entry;
  tfwrap_find_file_by_longname(&tf, L"big.txt", &entry);

  thinfat_open_file(&tf, &entry);

  size_t read;
#define READ_SIZE (10000000)
  char *read_buf = (char *)malloc(READ_SIZE);
  tfwrap_read_file(&tf, read_buf, READ_SIZE, &read);

  printf("%u bytes read.\n", read);

  //tfwrap_allocate_cluster(&tf, 50000);

  thinfat_phy_stop(&phy);

  thinfat_finalize(&tf);

  thinfat_phy_finalize(&phy);

  FILE *fp = fopen("dump.txt", "wb");
  printf("%u bytes read.\n", read);
  printf("%u\n", fwrite(read_buf, read, 1, fp));
  fclose(fp);

  return EXIT_SUCCESS;
}
