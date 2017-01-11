cmake_minimum_required(VERSION 2.8)
add_definitions("-Wall -Wextra -std=c99 -Wno-switch -g")
add_executable(demo main.c thinfat.c thinfat_blk.c thinfat_cache.c thinfat_phy_posix.c)
