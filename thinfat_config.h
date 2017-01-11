/*!
 * @file thinfat_config.h
 * @brief thinFAT configuration listing
 * @date 2017/01/12
 * @author Hiroka IHARA
 */
#ifndef THINFAT_CONFIG_H
#define THINFAT_CONFIG_H

#define THINFAT_CONFIG_ENABLE_LFN (1)

#if THINFAT_CONFIG_ENABLE_LFN
#include "wchar.h"
#if __SIZEOF_WCHAR_T__ != 2
#error "wchar_t is not 16bit."
#endif
#if !defined(__STDC_ISO_10646__)
#error "The compiler does not support UCS-2 encoding."
#endif
#endif

#endif
