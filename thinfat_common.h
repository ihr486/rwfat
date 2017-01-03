#ifndef THINFAT_COMMON_H
#define THINFAT_COMMON_H

#include <stdint.h>
#include <stdio.h>

#define THINFAT_SECTOR_SIZE (512)
#define THINFAT_INVALID_CLUSTER (0x0FFFFFF7)

#define THINFAT_IS_CLUSTER_VALID(c) ((c) < THINFAT_INVALID_CLUSTER)

#define THINFAT_INFO(...) printf(__VA_ARGS__)
#define THINFAT_ERROR(...) fprintf(stderr, __VA_ARGS__)

typedef uint32_t thinfat_sector_t;
typedef uint32_t thinfat_cluster_t;
typedef uint32_t thinfat_param_t;

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
