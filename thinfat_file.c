#include "thinfat.h"
#include "thinfat_file.h"

static thinfat_result_t thinfat_file_read_prepare_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param);
static thinfat_result_t thinfat_file_read_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param);
static thinfat_result_t thinfat_file_write_prepare_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param);
static thinfat_result_t thinfat_file_write_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param);
static thinfat_result_t thinfat_file_write_finish_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param);

thinfat_result_t thinfat_file_callback(thinfat_file_t *file, thinfat_core_event_t event, thinfat_sector_t s_param, void *p_param)
{
  switch(event)
  {
  case THINFAT_FILE_EVENT_READ_PREPARE:
    return thinfat_file_read_prepare_callback(file, s_param, p_param);
  case THINFAT_FILE_EVENT_READ:
    return thinfat_file_read_callback(file, s_param, p_param);
  case THINFAT_FILE_EVENT_WRITE_PREPARE:
    return thinfat_file_write_prepare_callback(file, s_param, p_param);
  case THINFAT_FILE_EVENT_WRITE:
    return thinfat_file_write_callback(file, s_param, p_param);
  case THINFAT_FILE_EVENT_WRITE_FINISH:
    return thinfat_file_write_finish_callback(file, s_param, p_param);
  }
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_file_read_prepare_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param)
{
  return thinfat_blk_read_each_cluster(file, &file->blk, file->sc_read, THINFAT_FILE_EVENT_READ);
}

static thinfat_result_t thinfat_file_read_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param)
{
  if (p_param == NULL)
  {
    return thinfat_core_callback(file->client, file->event, THINFAT_INVALID_SECTOR, &file->counter);
  }
  else if (*(void **)p_param == NULL)
  {
    if (file->blk.position % THINFAT_SECTOR_SIZE == 0)
    {
      *(void **)p_param = file->buffer;
    }
    else
    {
      *(void **)p_param = file->blk.cache->data;
    }
  }
  else
  {
    size_t advance;
    if (file->blk.position % THINFAT_SECTOR_SIZE > 0)
    {
      advance = THINFAT_SECTOR_SIZE - (file->blk.position % THINFAT_SECTOR_SIZE);
      if (advance > file->advance)
      {
        advance = file->advance;
      }
      memcpy(file->buffer, (uint8_t *)*(void **)p_param + (file->blk.position % THINFAT_SECTOR_SIZE), advance);
    }
    else
    {
      advance = THINFAT_SECTOR_SIZE;
      if (advance > file->advance)
      {
        advance = file->advance;
        memcpy(file->buffer, *(void **)p_param, advance);
      }
    }
    file->buffer = (uint8_t *)tf->cur_file->user_buffer + advance;
    file->advance -= advance;
    file->blk.position += advance;
    file->counter += advance;
    if (tf->cur_file->byte_advance >= THINFAT_SECTOR_SIZE)
    {
      *(void **)p_param = tf->cur_file->user_buffer;
    }
    else
    {
      *(void **)p_param = tf->cur_file->cache->data;
    }
  }
  //THINFAT_INFO("Read callback @ " TFF_X32 " = %p\n", s_param, *(void **)p_param);
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_file_write_prepare_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param)
{
  thinfat_sector_t sc_write = ((tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE) + tf->cur_file->byte_advance) / THINFAT_SECTOR_SIZE;
  size_t advance = THINFAT_SECTOR_SIZE - (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE);
  /*if (advance > tf->cur_file->byte_advance)
    {
    advance = tf->cur_file->byte_advance;
    }*/
  memcpy(*(uint8_t **)p_param + (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE), tf->cur_file->user_buffer, advance);
  //thinfat_cache_touch(tf->cur_file->cache);
  return thinfat_blk_write_each_cluster(tf, tf->cur_file, sc_write, THINFAT_FILE_EVENT_WRITE);
}

static thinfat_result_t thinfat_write_file_callback(thinfat_t *tf, thinfat_sector_t s_param, void *p_param)
{
  if (p_param == NULL)
  {
    size_t advance = THINFAT_SECTOR_SIZE - (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE);
    tf->cur_file->byte_pointer += advance;
    tf->cur_file->byte_advance -= advance;
    tf->cur_file->byte_counter += advance;
    tf->cur_file->user_buffer = (uint8_t *)tf->cur_file->user_buffer + advance;
    if (tf->cur_file->byte_advance > 0)
    {
      return thinfat_blk_read_each_sector(tf, tf->cur_file, 1, THINFAT_CORE_EVENT_WRITE_FILE_FINISH);
    }
    return thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, &tf->cur_file->byte_counter);
  }
  else if (*(void **)p_param == NULL)
  {
    if (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE == 0)
    {
      *(void **)p_param = tf->cur_file->user_buffer;
    }
    else
    {
      *(void **)p_param = tf->cur_file->cache->data;
    }
  }
  else
  {
    size_t advance = THINFAT_SECTOR_SIZE - (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE);
    tf->cur_file->byte_pointer += advance;
    tf->cur_file->byte_advance -= advance;
    tf->cur_file->byte_counter += advance;
    tf->cur_file->user_buffer = (uint8_t *)tf->cur_file->user_buffer + advance;
  }
  THINFAT_INFO("Write callback @ " TFF_X32 " = %p\n", s_param, *(void **)p_param);
  return THINFAT_RESULT_OK;
}

static thinfat_result_t thinfat_file_write_finish_callback(thinfat_file_t *file, thinfat_sector_t s_param, void *p_param)
{
  size_t advance = THINFAT_SECTOR_SIZE - (file->blk.position % THINFAT_SECTOR_SIZE);
  if (advance > file->byte_advance)
  {
    advance = file->byte_advance;
  }
  memcpy(*(uint8_t **)p_param + (tf->cur_file->byte_pointer % THINFAT_SECTOR_SIZE), tf->cur_file->user_buffer, advance);
  file->byte_pointer += advance;
  file->byte_counter += advance;
  //tf->cur_file->byte_advance -= advance;
  thinfat_cache_touch(file->blk.cache);
  return thinfat_core_callback(tf, tf->event, THINFAT_INVALID_SECTOR, &tf->cur_file->byte_counter);
}

thinfat_result_t thinfat_file_read(void *client, thinfat_file_t *file, void *buf, thinfat_size_t size, thinfat_core_event_t event)
{
  thinfat_t *tf = file->parent;

  thinfat_sector_t sc_read = ((file->blk.position % THINFAT_SECTOR_SIZE) + size + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
  thinfat_sector_t si_read = (file->so_current & ((1 << tf->ctos_shift) - 1)) + thinfat_ctos(tf, file->ci_current);
  if (size > file->size - file->blk.position)
    size = file->size - file->blk.position;

  file->advance = size;
  file->buffer = buf;
  file->sc_read = sc_read;
  file->counter = 0;
  file->event = event;
  file->client = client;

  if (file->blk.position % THINFAT_SECTOR_SIZE > 0 || (file->blk.position + size) % THINFAT_SECTOR_SIZE > 0)
  {
    return thinfat_cached_read_single(file, file->blk.cache, THINFAT_INVALID_SECTOR, THINFAT_FILE_EVENT_READ_PREPARE);
  }
  else
  {
    return thinfat_blk_read_each_cluster(file, &file->blk, si_read, THINFAT_FILE_EVENT_READ);
  }
}

thinfat_result_t thinfat_file_write(void *client, thinfat_file_t *file, const void *buf, thinfat_size_t size, thinfat_core_event_t event)
{
  thinfat_t *tf = (thinfat_t *)file->parent;

  thinfat_sector_t sc_write = ((file->blk.position % THINFAT_SECTOR_SIZE) + size + THINFAT_SECTOR_SIZE - 1) / THINFAT_SECTOR_SIZE;
  thinfat_sector_t si_write = (file->so_current & ((1 << tf->ctos_shift) - 1)) + thinfat_ctos(tf, file->blk.ci_current);

  file->advance = size;
  file->counter = 0;
  file->buffer = (void *)buf;
  file->event = event;
  file->client = client;

  if ((file->blk.position + size) % THINFAT_SECTOR_SIZE > 0)
  {
    sc_write--;
  }
  if (sc_write > 0)
  {
    if (file->blk.position % THINFAT_SECTOR_SIZE == 0)
    {
      return thinfat_blk_write_each_cluster(file, &file->blk, sc_write, THINFAT_FILE_EVENT_WRITE);
    }
    else
    {
      return thinfat_blk_read_each_sector(file, &file->blk, 1, THINFAT_FILE_EVENT_WRITE_PREPARE);
    }
  }
  else
  {
    return thinfat_blk_read_each_sector(file, &file->blk, 1, THINFAT_FILE_EVENT_WRITE_FINISH);
  }
}

