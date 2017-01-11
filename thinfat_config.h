#ifndef THINFAT_CONFIG_H
#define THINFAT_CONFIG_H

#define THINFAT_CONFIG_ENABLE_LFN (1)

#if THINFAT_CONFIG_ENABLE_LFN
#if !defined(__STDC_ISO_10646__)
#error "The compiler does not support UCS-2 encoding."
#endif
#endif

#endif
