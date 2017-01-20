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
  tfwrap_find_file_by_longname(&tf, L"output.txt", &entry);

  thinfat_open_file(&tf, &entry);

  FILE *fp = fopen("sample.pdf", "rb");
  size_t read, written;
#define WRITE_SIZE (10000000)
  char *write_buf = (char *)malloc(WRITE_SIZE);
  read = fread(write_buf, 1, WRITE_SIZE, fp);
  printf("Writing %u bytes...\n", read);
  if (tfwrap_write_file(&tf, write_buf, read, &written) != THINFAT_RESULT_OK)
  {
    fprintf(stderr, "Failed to write to file.\n");
    return -1;
  }
  fclose(fp);

  /*size_t read = 0;
#define READ_SIZE (10000000)
  char *read_buf = (char *)malloc(READ_SIZE);
  for (int i = 0; i < 1000; i++)
  {
    size_t read1;
    if (tfwrap_read_file(&tf, read_buf + READ_SIZE / 1000 * i, READ_SIZE / 1000, &read1) != THINFAT_RESULT_OK)
    {
      fprintf(stderr, "Failed to read from file.\n");
      return -1;
    }
    read += read1;
  }

  printf("%u bytes read.\n", read);

  //tfwrap_allocate_cluster(&tf, 50000);

  thinfat_phy_stop(&phy);

  thinfat_finalize(&tf);

  thinfat_phy_finalize(&phy);

  FILE *fp = fopen("dump.txt", "wb");
  printf("%u bytes read.\n", read);
  printf("%u\n", fwrite(read_buf, read, 1, fp));
  fclose(fp);*/

  return EXIT_SUCCESS;
}
