#ifndef THINFAT_COMMON_H
#define THINFAT_COMMON_H

#include <stdint.h>
#include <stdio.h>

#define THINFAT_IS_CLUSTER_VALID(c) ((c) < THINFAT_INVALID_CLUSTER)

#if __SIZEOF_INT__ == 4
#define TFF_I32 "%d"
#define TFF_U32 "%u"
#define TFF_X32 "0x%08X"
#define THINFAT_SECTOR_SIZE (512U)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7U)
#elif __SIZEOF_LONG__ == 4
#define TFF_I32 "%ld"
#define TFF_U32 "%lu"
#define TFF_X32 "0x%08lX"
#define THINFAT_SECTOR_SIZE (512UL)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7UL)
#elif __SIZEOF_LONGLONG__ == 4
#define TFF_I32 "%lld"
#define TFF_U32 "%llu"
#define TFF_X32 "0x%08llX"
#define THINFAT_SECTOR_SIZE (512ULL)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7ULL)
#else
#error "The compiler does not support 32bit integer."
#endif

#define THINFAT_INFO(...) printf(__VA_ARGS__)
#define THINFAT_ERROR(...) fprintf(stderr, __VA_ARGS__)

typedef uint32_t thinfat_sector_t;
typedef uint32_t thinfat_cluster_t;

typedef enum
{
  THINFAT_EVENT_FIND_PARTITION,
  THINFAT_EVENT_MOUNT,
  THINFAT_EVENT_UNMOUNT
}
thinfat_event_t;

typedef enum
{
  THINFAT_RESULT_OK = 0,
  THINFAT_RESULT_PHY_ERROR,   //Generic or unidentifiable error in the PHY
  THINFAT_RESULT_PHY_BUSY,    //PHY is waiting for another operation
  THINFAT_RESULT_ERROR_SIGNATURE,
  THINFAT_RESULT_NO_PARTITION,
  THINFAT_RESULT_ERROR_BPB
}
thinfat_result_t;

#endif
