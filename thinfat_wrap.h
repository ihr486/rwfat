#ifndef THINFAT_WRAP_H
#define THINFAT_WRAP_H

#include "thinfat.h"

typedef thinfat_result_t tfwrap_result_t;

tfwrap_result_t tfwrap_find_partition(thinfat_t *tf);
tfwrap_result_t tfwrap_mount(thinfat_t *tf, thinfat_sector_t si);
tfwrap_result_t tfwrap_unmount(thinfat_t *tf);

#endif
