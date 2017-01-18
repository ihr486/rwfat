/*!
 * @file thinfat_dir.c
 * @brief thinFAT DIR layer implementation
 * @date 2017/01/18
 * @author Hiroka IHARA
 */
#include "thinfat.h"
#include "thinfat_dir.h"

#include <string.h>

#define THINFAT_ATTR_READ_ONLY (0x01)
#define THINFAT_ATTR_HIDDEN (0x02)
#define THINFAT_ATTR_SYSTEM (0x04)
#define THINFAT_ATTR_VOLUME_ID (0x08)
#define THINFAT_ATTR_DIRECTORY (0x10)
#define THINFAT_ATTR_ARCHIVE (0x20)
#define THINFAT_ATTR_LONG_FILE_NAME (0x0F)

static thinfat_result_t thinfat_dir_dump_callback(thinfat_dir_t *dir, void *entries);
static thinfat_result_t thinfat_dir_find_callback(thinfat_dir_t *dir, void *entries);
static thinfat_result_t thinfat_dir_find_by_longname_callback(thinfat_dir_t *dir, void *entries);

thinfat_result_t thinfat_dir_callback(thinfat_dir_t *dir, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_DIR_EVENT_DUMP:
    return thinfat_dir_dump_callback(dir, *(void **)p_param);
  case THINFAT_DIR_EVENT_FIND:
    return thinfat_dir_find_callback(dir, *(void **)p_param);
  case THINFAT_DIR_EVENT_FIND_BY_LONGNAME:
    return thinfat_dir_find_by_longname_callback(dir, *(void **)p_param);
  }
  return THINFAT_RESULT_OK;
}

static size_t thinfat_wstrlen(const wchar_t *str)
{
  size_t ret;
  for (ret = 0; str[ret] != L'\0'; ret++);
  return ret;
}

static inline thinfat_sector_t thinfat_root_sector_count(thinfat_t *tf)
{
  return (tf->root_entry_count * 32 + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
}

static thinfat_dir_entry_t *thinfat_decode_dir_entry(uint8_t *src, thinfat_dir_entry_t *dest)
{
  dest->attr = thinfat_read_u8(src, 11);
  memcpy(dest->name, src, 11);
  dest->name[11] = '\0';
  dest->ci_head = ((uint32_t)thinfat_read_u16(src, 20) << 16) | thinfat_read_u16(src, 26);
  dest->size = thinfat_read_u32(src, 28);
  return dest;
}

static thinfat_lfn_entry_t *thinfat_decode_lfn_entry(uint8_t *src, thinfat_lfn_entry_t *dest)
{
  dest->order = src[0];
  dest->checksum = src[13];
  for (int i = 0; i < 5; i++)
    dest->partial_name[i] = thinfat_read_u16(src, i * 2 + 1);
  for (int i = 0; i < 6; i++)
    dest->partial_name[i + 5] = thinfat_read_u16(src, i * 2 + 14);
  for (int i = 0; i < 2; i++)
    dest->partial_name[i + 11] = thinfat_read_u16(src, i * 2 + 28);
  dest->partial_name[13] = L'\0';
  return dest;
}

static thinfat_result_t thinfat_dir_traverse(thinfat_dir_t *dir, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)dir->parent;
  thinfat_sector_t sc_read;
  if (dir->blk.ci_head == tf->ci_root)
    sc_read = thinfat_root_sector_count(tf);
  else
    sc_read = 0xFFFFFFFF;
  thinfat_blk_rewind(&dir->blk);
  return thinfat_blk_read_each_sector(dir, &dir->blk, 0, sc_read, event);
}

thinfat_result_t thinfat_dir_dump(void *client, thinfat_dir_t *dir, thinfat_core_event_t event)
{
  dir->client = client;
  dir->event = event;
  return thinfat_dir_traverse(dir, THINFAT_DIR_EVENT_DUMP);
}

thinfat_result_t thinfat_dir_find(void *client, thinfat_dir_t *dir, const char *name, thinfat_core_event_t event)
{
  dir->client = client;
  dir->event = event;
  dir->target_name = (const void *)name;
  return thinfat_dir_traverse(dir, THINFAT_DIR_EVENT_FIND);
}

thinfat_result_t thinfat_dir_find_by_longname(void *client, thinfat_dir_t *dir, const wchar_t *name, thinfat_core_event_t event)
{
  dir->client = client;
  dir->event = event;
  dir->target_name = (const void *)name;
  dir->nc_matched = 0;
  return thinfat_dir_traverse(dir, THINFAT_DIR_EVENT_FIND_BY_LONGNAME);
}

static thinfat_result_t thinfat_dir_dump_callback(thinfat_dir_t *dir, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(dir->client, dir->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0xE5)
      {
        if (entry.attr == THINFAT_ATTR_LONG_FILE_NAME)
        {
          if (entry.name[0] & 0x40)
          {
            THINFAT_INFO("LFN[%d] (LAST)\n", entry.name[0] & 0x3F);
          }
          else
          {
            THINFAT_INFO("LFN[%d]\n", entry.name[0]);
          }
        }
        else if (entry.name[0] != 0x05)
        {
          THINFAT_INFO("\"%s\" @ " TFF_X32 " * " TFF_U32 "\n", entry.name, entry.ci_head, entry.size);
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_dir_find_callback(thinfat_dir_t *dir, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(dir->client, dir->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0x05 && entry.name[0] != 0xE5)
      {
        if (!(entry.attr & (THINFAT_ATTR_HIDDEN | THINFAT_ATTR_SYSTEM)))
        {
          unsigned int j;
          for (j = 0; j < 11; j++)
          {
            if (entry.name[j] != ((const char *)dir->target_name)[j]) break;
          }
          if (j == 11)
          {
            thinfat_core_callback(dir->client, dir->event, THINFAT_INVALID_SECTOR, &entry);
            return THINFAT_RESULT_ABORT;
          }
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_dir_find_by_longname_callback(thinfat_dir_t *dir, void *entries)
{
  if (entries == NULL)
  {
    return thinfat_core_callback(dir->client, dir->event, THINFAT_INVALID_SECTOR, NULL);
  }
  else
  {
    for (unsigned int i = 0; i < THINFAT_SECTOR_SIZE / 32; i++)
    {
      thinfat_dir_entry_t entry;
      thinfat_decode_dir_entry((uint8_t *)entries + i * 32, &entry);
      if (entry.name[0] != 0x00 && entry.name[0] != 0xE5)
      {
        if (entry.attr == THINFAT_ATTR_LONG_FILE_NAME)
        {
          thinfat_lfn_entry_t lfn;
          thinfat_decode_lfn_entry((uint8_t *)entries + i * 32, &lfn);
          if (lfn.order & 0x40)
            dir->nc_matched = 0;
          else if (lfn.checksum != dir->checksum)
            dir->nc_matched = 0;
          else if (dir->nc_matched > 255)
            dir->nc_matched = 0;
          unsigned int ic_match = 13 * (entry.name[0] & 0x1F) - 13;
          unsigned int j;
          const wchar_t *target_name = (const wchar_t *)dir->target_name;
          unsigned int nc_target = thinfat_wstrlen(target_name);
          for (j = 0; j < 13 && ic_match + j < nc_target; j++)
          {
            if (target_name[ic_match + j] != lfn.partial_name[j]) break;
          }
          dir->nc_matched += j;
          if (dir->nc_matched == nc_target)
          {
            dir->nc_matched = 256;
          }
          dir->checksum = lfn.checksum;
        }
        else if (!(entry.attr & (THINFAT_ATTR_HIDDEN | THINFAT_ATTR_SYSTEM)) && entry.name[0] != 0x05)
        {
          if (dir->nc_matched > 255)
          {
            dir->nc_matched = 0;
            thinfat_core_callback(dir->client, dir->event, THINFAT_INVALID_SECTOR, &entry);
            return THINFAT_RESULT_ABORT;
          }
        }
      }
    }
  }
  return THINFAT_RESULT_OK;
}

