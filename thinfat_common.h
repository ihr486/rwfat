/*!
 * @file thinfat_common.h
 * @brief Common definitions for RivieraWaves FAT driver
 * @date 2017/01/09
 * @author Hiroka IHARA
 */

#ifndef THINFAT_COMMON_H
#define THINFAT_COMMON_H

#include <stdint.h>
#include <stdio.h>

#define THINFAT_IS_CLUSTER_VALID(c) ((c) < THINFAT_INVALID_CLUSTER)
#define THINFAT_IS_SECTOR_VALID(c) ((c) != THINFAT_INVALID_SECTOR)

#if __SIZEOF_INT__ == 4
#define TFF_I32 "%d"
#define TFF_U32 "%u"
#define TFF_X32 "0x%08X"
#define THINFAT_SECTOR_SIZE (512U)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7U)
#define THINFAT_INVALID_SECTOR (0U)
#elif __SIZEOF_LONG__ == 4
#define TFF_I32 "%ld"
#define TFF_U32 "%lu"
#define TFF_X32 "0x%08lX"
#define THINFAT_SECTOR_SIZE (512UL)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7UL)
#define THINFAT_INVALID_SECTOR (0UL)
#elif __SIZEOF_LONGLONG__ == 4
#define TFF_I32 "%lld"
#define TFF_U32 "%llu"
#define TFF_X32 "0x%08llX"
#define THINFAT_SECTOR_SIZE (512ULL)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7ULL)
#define THINFAT_INVALID_SECTOR (0ULL)
#else
#error "The compiler does not support 32bit integer."
#endif

#define THINFAT_INFO(...) printf(__VA_ARGS__)
#define THINFAT_ERROR(...) fprintf(stderr, __VA_ARGS__)

typedef uint32_t thinfat_sector_t;
typedef uint32_t thinfat_cluster_t;

typedef enum
{
  THINFAT_RESULT_OK = 0,
  THINFAT_RESULT_ABORT,       //Not an error; just terminate current operation
  THINFAT_RESULT_PHY_ERROR,   //Generic or unidentifiable error in the PHY
  THINFAT_RESULT_PHY_BUSY,    //PHY is waiting for another operation
  THINFAT_RESULT_ERROR_SIGNATURE,
  THINFAT_RESULT_NO_PARTITION,
  THINFAT_RESULT_ERROR_BPB,
  THINFAT_RESULT_BLK_BUSY,
  THINFAT_RESULT_EOF
}
thinfat_result_t;

typedef enum
{
  THINFAT_CORE_EVENT_NONE = 0,
  THINFAT_CORE_EVENT_READ_MBR,
  THINFAT_CORE_EVENT_READ_BPB,
  THINFAT_CORE_EVENT_DUMP_DIR,
  THINFAT_CORE_EVENT_FIND_FILE,
  THINFAT_CORE_EVENT_MAX,
  THINFAT_CACHE_EVENT_READ,
  THINFAT_CACHE_EVENT_WRITE,
  THINFAT_CACHE_EVENT_MAX,
  THINFAT_BLK_EVENT_LOOKUP,
  THINFAT_BLK_EVENT_READ_SINGLE,
  THINFAT_BLK_EVENT_READ_SINGLE_LOOKUP,
  THINFAT_BLK_EVENT_READ_CLUSTER,
  THINFAT_BLK_EVENT_READ_CLUSTER_LOOKUP,
  THINFAT_BLK_EVENT_WRITE_CLUSTER,
  THINFAT_BLK_EVENT_WRITE_CLUSTER_LOOKUP,
  THINFAT_BLK_EVENT_MAX,
  THINFAT_EVENT_FIND_PARTITION,
  THINFAT_EVENT_MOUNT,
  THINFAT_EVENT_UNMOUNT,
  THINFAT_EVENT_DUMP_DIR_COMPLETE,
  THINFAT_EVENT_FIND_FILE_COMPLETE,
  THINFAT_EVENT_MAX,
  THINFAT_USER_EVENT
}
thinfat_core_event_t;

typedef thinfat_core_event_t thinfat_event_t;

#endif
