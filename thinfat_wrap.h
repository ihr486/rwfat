/*!
 * @file thinfat_wrap.h
 * @brief thinFAT synchronous wrapper interface
 * @date 2017/01/12
 * @author Hiroka IHARA
 */
#ifndef THINFAT_WRAP_H
#define THINFAT_WRAP_H

#include "thinfat.h"

typedef thinfat_result_t tfwrap_result_t;

tfwrap_result_t tfwrap_find_partition(thinfat_t *tf);
tfwrap_result_t tfwrap_mount(thinfat_t *tf, thinfat_sector_t si);
tfwrap_result_t tfwrap_unmount(thinfat_t *tf);
tfwrap_result_t tfwrap_dump_current_directory(thinfat_t *tf);
tfwrap_result_t tfwrap_find_file(thinfat_t *tf, const char *name, thinfat_dir_entry_t *entry);
tfwrap_result_t tfwrap_find_file_by_longname(thinfat_t *tf, const wchar_t *longname, thinfat_dir_entry_t *entry);
tfwrap_result_t tfwrap_read_file(thinfat_t *tf, void *buf, size_t size, size_t *read);
tfwrap_result_t tfwrap_write_file(thinfat_t *tf, const void *buf, size_t size, size_t *written);

tfwrap_result_t tfwrap_allocate_cluster(thinfat_t *tf, thinfat_cluster_t cc_allocate);

#endif
